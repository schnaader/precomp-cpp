/* Copyright 2018 Dirk Steinke

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License. */

#include <string.h>
#include <functional>
#include "preflate_block_decoder.h"
#include "preflate_decoder.h"
#include "preflate_parameter_estimator.h"
#include "preflate_statistical_model.h"
#include "preflate_token_predictor.h"
#include "preflate_tree_predictor.h"
#include "support/bitstream.h"
#include "support/memstream.h"
#include "support/outputcachestream.h"

class PreflateDecoderHandler : public PreflateDecoderTask::Handler {
public:
  PreflateDecoderHandler(std::function<void(void)> progressCallback_)
    : progressCallback(progressCallback_) {}

  bool finish(std::vector<uint8_t>& reconstructionData) {
    reconstructionData = encoder.finish();
    return !encoder.error();
  }
  bool error() const {
    return encoder.error();
  }

  virtual uint32_t setModel(const PreflateStatisticsCounter& counters, const PreflateParameters& parameters) {
    return encoder.addModel(counters, parameters);
  }
  virtual bool beginEncoding(const uint32_t metaBlockId, PreflatePredictionEncoder& codec, const uint32_t modelId) {
    return encoder.beginMetaBlockWithModel(codec, modelId);
  }
  virtual bool endEncoding(const uint32_t metaBlockId, PreflatePredictionEncoder& codec, const size_t uncompressedSize) {
    return encoder.endMetaBlock(codec, uncompressedSize);
  }
  virtual void markProgress() {
    std::unique_lock<std::mutex> lock(this->_mutex);
    progressCallback();
  }

private:
  PreflateMetaEncoder encoder;
  std::function<void(void)> progressCallback;
  std::mutex _mutex;
};

PreflateDecoderTask::PreflateDecoderTask(PreflateDecoderTask::Handler& handler_,
                                         const uint32_t metaBlockId_,
                                         std::vector<PreflateTokenBlock>&& tokenData_,
                                         std::vector<uint8_t>&& uncompressedData_,
                                         const size_t uncompressedOffset_,
                                         const bool lastMetaBlock_,
                                         const uint32_t paddingBits_)
  : handler(handler_)
  , metaBlockId(metaBlockId_)
  , tokenData(tokenData_)
  , uncompressedData(uncompressedData_)
  , uncompressedOffset(uncompressedOffset_)
  , lastMetaBlock(lastMetaBlock_)
  , paddingBits(paddingBits_) {
}

bool PreflateDecoderTask::analyze() {
  params = estimatePreflateParameters(uncompressedData, uncompressedOffset, tokenData);
  memset(&counter, 0, sizeof(counter));
  tokenPredictor.reset(new PreflateTokenPredictor(params, uncompressedData, uncompressedOffset));
  treePredictor.reset(new PreflateTreePredictor(uncompressedData, uncompressedOffset));
  for (unsigned i = 0, n = tokenData.size(); i < n; ++i) {
    tokenPredictor->analyzeBlock(i, tokenData[i]);
    treePredictor->analyzeBlock(i, tokenData[i]);
    if (tokenPredictor->predictionFailure || treePredictor->predictionFailure) {
      return false;
    }
    tokenPredictor->updateCounters(&counter, i);
    treePredictor->updateCounters(&counter, i);
    handler.markProgress();
  }
  counter.block.incNonZeroPadding(paddingBits != 0);
  return true;
}

bool PreflateDecoderTask::encode() {
  PreflatePredictionEncoder pcodec;
  unsigned modelId = handler.setModel(counter, params);
  if (!handler.beginEncoding(metaBlockId, pcodec, modelId)) {
    return false;
  }
  for (unsigned i = 0, n = tokenData.size(); i < n; ++i) {
    tokenPredictor->encodeBlock(&pcodec, i);
    treePredictor->encodeBlock(&pcodec, i);
    if (tokenPredictor->predictionFailure || treePredictor->predictionFailure) {
      return false;
    }
    if (lastMetaBlock) {
      tokenPredictor->encodeEOF(&pcodec, i, i + 1 == tokenData.size());
    }
  }
  if (lastMetaBlock) {
    pcodec.encodeNonZeroPadding(paddingBits != 0);
    if (paddingBits != 0) {
      unsigned bitsToSave = bitLength(paddingBits);
      pcodec.encodeValue(bitsToSave, 3);
      if (bitsToSave > 1) {
        pcodec.encodeValue(paddingBits & ((1 << (bitsToSave - 1)) - 1), bitsToSave - 1);
      }
    }
  }
  return handler.endEncoding(metaBlockId, pcodec, uncompressedData.size() - uncompressedOffset);
}

bool preflate_decode(OutputStream& unpacked_output,
                     std::vector<unsigned char>& preflate_diff,
                     uint64_t& deflate_size,
                     InputStream& deflate_raw,
                     std::function<void(void)> block_callback,
                     const size_t min_deflate_size,
                     const size_t metaBlockSize) {
  deflate_size = 0;
  uint64_t deflate_bits = 0;
  size_t prevBitPos = 0;
  BitInputStream decInBits(deflate_raw);
  OutputCacheStream decOutCache(unpacked_output);
  PreflateBlockDecoder bdec(decInBits, decOutCache);
  if (bdec.status() != PreflateBlockDecoder::OK) {
    return false;
  }
  bool last;
  unsigned i = 0;
  std::vector<PreflateTokenBlock> blocks;
  std::vector<uint32_t> blockSizes;
  uint64_t sumBlockSizes = 0;
  uint64_t lastEndPos = 0;
  uint64_t uncompressedMetaStart = 0;
  size_t MBSize = std::min<size_t>(std::max<size_t>(metaBlockSize, 1u << 18), (1u << 31) - 1);
  size_t MBThreshold = (MBSize * 3) >> 1;
  PreflateDecoderHandler encoder(block_callback);
  size_t MBcount = 0;

  std::queue<std::future<std::shared_ptr<PreflateDecoderTask>>> futureQueue;
  size_t queueLimit = std::min(2 * globalTaskPool.extraThreadCount(), (1 << 26) / MBThreshold);
  bool fail = false;

  do {
    PreflateTokenBlock newBlock;

    bool ok = bdec.readBlock(newBlock, last);
    if (!ok) {
      fail = true;
      break;
    }

    uint64_t blockSize = decOutCache.cacheEndPos() - lastEndPos;
    lastEndPos = decOutCache.cacheEndPos();
    if (blockSize >= (1 << 31)) {
      // No mega blocks
      fail = true;
      break;
    }

    blocks.push_back(newBlock);
    blockSizes.push_back(blockSize);
    ++i;
    block_callback();

    deflate_bits += decInBits.bitPos() - prevBitPos;
    prevBitPos = decInBits.bitPos();

    sumBlockSizes += blockSize;
    if (last || sumBlockSizes >= MBThreshold) {
      size_t blockCount, blockSizeSum;
      if (last) {
        blockCount = blockSizes.size();
        blockSizeSum = sumBlockSizes;
      } else {
        blockCount = 0;
        blockSizeSum = 0;
        for (const auto bs : blockSizes) {
          blockSizeSum += bs;
          ++blockCount;
          if (blockSizeSum >= MBSize) {
            break;
          }
        }
      }
      std::vector<PreflateTokenBlock> blocksForMeta;
      for (size_t j = 0; j < blockCount; ++j) {
        blocksForMeta.push_back(std::move(blocks[j]));
      }
      blocks.erase(blocks.begin(), blocks.begin() + blockCount);
      blockSizes.erase(blockSizes.begin(), blockSizes.begin() + blockCount);
      sumBlockSizes -= blockSizeSum;

      size_t uncompressedOffset = MBcount == 0 ? 0 : 1 << 15;

      std::vector<uint8_t> uncompressedDataForMeta(
        decOutCache.cacheData(uncompressedMetaStart - uncompressedOffset),
        decOutCache.cacheData(uncompressedMetaStart - uncompressedOffset) + blockSizeSum + uncompressedOffset);
      uncompressedMetaStart += blockSizeSum;

      size_t paddingBits = 0;
      if (last) {
        uint8_t remaining_bit_count = (8 - deflate_bits) & 7;
        paddingBits = decInBits.get(remaining_bit_count);

        deflate_bits += decInBits.bitPos() - prevBitPos;
        prevBitPos = decInBits.bitPos();
      }
      if (futureQueue.empty() && (queueLimit == 0 || last)) {
        PreflateDecoderTask task(encoder, MBcount,
                                 std::move(blocksForMeta),
                                 std::move(uncompressedDataForMeta),
                                 uncompressedOffset,
                                 last, paddingBits);
        if (!task.analyze() || !task.encode()) {
          fail = true;
          break;
        }
      } else {
        if (futureQueue.size() >= queueLimit) {
          std::future<std::shared_ptr<PreflateDecoderTask>> first = std::move(futureQueue.front());
          futureQueue.pop();
          std::shared_ptr<PreflateDecoderTask> data = first.get();
          if (!data || !data->encode()) {
            fail = true;
            break;
          }
        }
        std::shared_ptr<PreflateDecoderTask> ptask;
        ptask.reset(new PreflateDecoderTask(encoder, MBcount,
                                            std::move(blocksForMeta),
                                            std::move(uncompressedDataForMeta),
                                            uncompressedOffset,
                                            last, paddingBits));
        futureQueue.push(globalTaskPool.addTask([ptask,&fail]() {
          if (!fail && ptask->analyze()) {
            return ptask;
          } else {
            return std::shared_ptr<PreflateDecoderTask>();
          }
        }));
      }
      if (!last) {
        decOutCache.flushUpTo(uncompressedMetaStart - (1 << 15));
      }
      MBcount++;
    }
  } while (!fail && !last);
  while (!futureQueue.empty()) {
    std::future<std::shared_ptr<PreflateDecoderTask>> first = std::move(futureQueue.front());
    futureQueue.pop();
    std::shared_ptr<PreflateDecoderTask> data = first.get();
    if (fail || !data || !data->encode()) {
      fail = true;
    }
  }
  decOutCache.flush();
  deflate_size = (deflate_bits + 7) >> 3;
  if (deflate_size < min_deflate_size) {
    return false;
  }
  return !fail && encoder.finish(preflate_diff);
}

bool preflate_decode(std::vector<unsigned char>& unpacked_output,
                     std::vector<unsigned char>& preflate_diff,
                     uint64_t& deflate_size,
                     InputStream& deflate_raw,
                     std::function<void(void)> block_callback,
                     const size_t min_deflate_size,
                     const size_t metaBlockSize) {
  MemStream uncompressedOutput;
  bool result = preflate_decode(uncompressedOutput, preflate_diff, deflate_size, deflate_raw,
                                block_callback, min_deflate_size, metaBlockSize);
  unpacked_output = uncompressedOutput.extractData();
  return result;
}

bool preflate_decode(std::vector<unsigned char>& unpacked_output,
                     std::vector<unsigned char>& preflate_diff,
                     const std::vector<unsigned char>& deflate_raw,
                     const size_t metaBlockSize) {
  MemStream mem(deflate_raw);
  uint64_t raw_size;
  return preflate_decode(unpacked_output, preflate_diff,
                         raw_size, mem, [] {}, 0, metaBlockSize) 
          && raw_size == deflate_raw.size();
}
