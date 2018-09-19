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

#include <functional>
#include "preflate_block_reencoder.h"
#include "preflate_reencoder.h"
#include "preflate_statistical_codec.h"
#include "preflate_token_predictor.h"
#include "preflate_tree_predictor.h"
#include "support/bitstream.h"
#include "support/memstream.h"

class PreflateReencoderHandler : public PreflateReencoderTask::Handler {
public:
  PreflateReencoderHandler(BitOutputStream& bos_,
                           const std::vector<uint8_t>& reconData,
                           const size_t uncompressedSize,
                           std::function<void(void)> progressCallback_)
    : decoder(reconData, uncompressedSize)
    , progressCallback(progressCallback_)
    , bos(bos_) {}

  size_t metaBlockCount() const {
    return decoder.metaBlockCount();
  }
  size_t metaBlockUncompressedSize(const size_t metaBlockId) const {
    return decoder.metaBlockUncompressedSize(metaBlockId);
  }
  bool error() const {
    return decoder.error();
  }

  bool finish() {
    decoder.finish();
    return !decoder.error();
  }

  virtual bool beginDecoding(const uint32_t metaBlockId, 
                             PreflatePredictionDecoder& codec, PreflateParameters& params) {
    return decoder.beginMetaBlock(codec, params, metaBlockId);
  }
  virtual bool endDecoding(const uint32_t metaBlockId, PreflatePredictionDecoder& codec,
                           std::vector<PreflateTokenBlock>&& tokenData,
                           std::vector<uint8_t>&& uncompressedData,
                           const size_t uncompressedOffset,
                           const size_t paddingBitCount,
                           const size_t paddingValue) {
    if (!decoder.endMetaBlock(codec)) {
      return false;
    }

    PreflateBlockReencoder deflater(bos, uncompressedData, uncompressedOffset);
    for (size_t j = 0, n = tokenData.size(); j < n; ++j) {
      deflater.writeBlock(tokenData[j],
                          metaBlockId + 1 == decoder.metaBlockCount() && j + 1 == n);
      markProgress();
    }
    bos.put(paddingValue, paddingBitCount);
    return true;
  }

  virtual void markProgress() {
    std::unique_lock<std::mutex> lock(this->_mutex);
    progressCallback();
  }

private:
  PreflateMetaDecoder decoder;
  std::function<void(void)> progressCallback;
  BitOutputStream& bos;
  std::mutex _mutex;
};

PreflateReencoderTask::PreflateReencoderTask(PreflateReencoderHandler::Handler& handler_,
                                             const uint32_t metaBlockId_,
                                             std::vector<uint8_t>&& uncompressedData_,
                                             const size_t uncompressedOffset_,
                                             const bool lastMetaBlock_)
  : handler(handler_)
  , metaBlockId(metaBlockId_)
  , uncompressedData(uncompressedData_)
  , uncompressedOffset(uncompressedOffset_)
  , lastMetaBlock(lastMetaBlock_) {}

bool PreflateReencoderTask::decodeAndRepredict() {
  PreflateParameters params;
  if (!handler.beginDecoding(metaBlockId, pcodec, params)) {
    return false;
  }
  PreflateTokenPredictor tokenPredictor(params, uncompressedData, uncompressedOffset);
  PreflateTreePredictor treePredictor(uncompressedData, uncompressedOffset);

  bool eof = true;
  do {
    PreflateTokenBlock block = tokenPredictor.decodeBlock(&pcodec);
    if (!treePredictor.decodeBlock(block, &pcodec)) {
      return false;
    }
    if (tokenPredictor.predictionFailure || treePredictor.predictionFailure) {
      return false;
    }
    tokenData.push_back(std::move(block));
    if (!lastMetaBlock) {
      eof = tokenPredictor.inputEOF();
    } else {
      eof = tokenPredictor.decodeEOF(&pcodec);
    }
    handler.markProgress();
  } while (!eof);
  paddingBitCount = 0;
  paddingBits = 0;
  if (lastMetaBlock) {
    bool non_zero_bits = pcodec.decodeNonZeroPadding();
    if (non_zero_bits) {
      paddingBitCount = pcodec.decodeValue(3);
      if (paddingBitCount > 0) {
        paddingBits = (1 << (paddingBitCount - 1)) + pcodec.decodeValue(paddingBitCount - 1);
      }
    }
  }
  return true;
}
bool PreflateReencoderTask::reencode() {
  return handler.endDecoding(metaBlockId, pcodec, std::move(tokenData),
                             std::move(uncompressedData), uncompressedOffset,
                             paddingBitCount, paddingBits);
}

bool preflate_reencode(OutputStream& os,
                       const std::vector<unsigned char>& preflate_diff,
                       InputStream& is,
                       const uint64_t unpacked_size,
                       std::function<void(void)> block_callback) {
  BitOutputStream bos(os);
  PreflateReencoderHandler decoder(bos, preflate_diff, unpacked_size, block_callback);
  if (decoder.error()) {
    return false;
  }
  std::vector<uint8_t> uncompressedData;
  std::queue<std::future<std::shared_ptr<PreflateReencoderTask>>> futureQueue;
  size_t maxMetaBlockSize = 1;
  for (size_t j = 0, n = decoder.metaBlockCount(); j < n; ++j) {
    maxMetaBlockSize = std::max(maxMetaBlockSize, decoder.metaBlockUncompressedSize(j));
  }
  size_t queueLimit = std::min(2 * globalTaskPool.extraThreadCount(), (1 << 26) / maxMetaBlockSize);
  bool fail = false;
  for (size_t j = 0, n = decoder.metaBlockCount(); j < n; ++j) {
    size_t curUncSize = uncompressedData.size();
    size_t newSize = decoder.metaBlockUncompressedSize(j);
    uncompressedData.resize(curUncSize + newSize);
    if (is.read(uncompressedData.data() + curUncSize, newSize) != newSize) {
      return false;
    }

    if (futureQueue.empty() && (queueLimit == 0 || j + 1 == n)) {
      PreflateReencoderTask task(decoder, j, std::vector<uint8_t>(uncompressedData), curUncSize, j + 1 == n);
      if (!task.decodeAndRepredict() || !task.reencode()) {
        return false;
      }
    } else {
      if (futureQueue.size() >= queueLimit) {
        std::future<std::shared_ptr<PreflateReencoderTask>> first = std::move(futureQueue.front());
        futureQueue.pop();
        std::shared_ptr<PreflateReencoderTask> data = first.get();
        if (fail || !data || !data->reencode()) {
          fail = true;
        }
      }
      std::shared_ptr<PreflateReencoderTask> ptask;
      ptask.reset(new PreflateReencoderTask(decoder, j, std::vector<uint8_t>(uncompressedData),
                                            curUncSize, j + 1 == n));
      futureQueue.push(globalTaskPool.addTask([ptask, &fail]() {
        if (!fail && ptask->decodeAndRepredict()) {
          return ptask;
        } else {
          return std::shared_ptr<PreflateReencoderTask>();
        }
      }));
    }

    if (j + 1 < n) {
      uncompressedData.erase(uncompressedData.begin(),
                             uncompressedData.begin() + std::max<size_t>(uncompressedData.size(), 1 << 15) - (1 << 15));
    }
  }
  while (!futureQueue.empty()) {
    std::future<std::shared_ptr<PreflateReencoderTask>> first = std::move(futureQueue.front());
    futureQueue.pop();
    std::shared_ptr<PreflateReencoderTask> data = first.get();
    if (fail || !data || !data->reencode()) {
      fail = true;
    }
  }
  bos.flush();
  return !fail && !decoder.error();
}

bool preflate_reencode(OutputStream& os,
                       const std::vector<unsigned char>& preflate_diff,
                       const std::vector<unsigned char>& unpacked_input,
                       std::function<void(void)> block_callback) {
  MemStream is(unpacked_input);
  return preflate_reencode(os, preflate_diff, is, unpacked_input.size(), block_callback);
}
bool preflate_reencode(std::vector<unsigned char>& deflate_raw,
                       const std::vector<unsigned char>& preflate_diff,
                       const std::vector<unsigned char>& unpacked_input) {
  MemStream mem;
  bool result = preflate_reencode(mem, preflate_diff, unpacked_input, [] {});
  deflate_raw = mem.extractData();
  return result;
}
