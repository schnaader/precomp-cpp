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

#ifndef PREFLATE_DECODER_H
#define PREFLATE_DECODER_H

#include <functional>
#include <queue>
#include <vector>
#include "preflate_statistical_codec.h"
#include "preflate_token.h"
#include "support/stream.h"
#include "support/task_pool.h"

class PreflateTokenPredictor;
class PreflateTreePredictor;

class PreflateDecoderTask {
public:
  class Handler {
  public:
    virtual uint32_t setModel(const PreflateStatisticsCounter&, const PreflateParameters&) = 0;
    virtual bool beginEncoding(const uint32_t metaBlockId, PreflatePredictionEncoder&, const uint32_t modelId) = 0;
    virtual bool endEncoding(const uint32_t metaBlockId, PreflatePredictionEncoder&, const size_t uncompressedSize) = 0;
    virtual void markProgress() = 0;
  };

  PreflateDecoderTask(Handler& handler,
                      const uint32_t metaBlockId, 
                      std::vector<PreflateTokenBlock>&& tokenData,
                      std::vector<uint8_t>&& uncompressedData,
                      const size_t uncompressedOffset,
                      const bool lastMetaBlock,
                      const uint32_t paddingBits);

  bool analyze();
  bool encode();
  uint32_t id() {
    return metaBlockId;
  }

private:
  Handler& handler;
  uint32_t metaBlockId;
  std::vector<PreflateTokenBlock> tokenData;
  std::vector<uint8_t> uncompressedData;
  size_t uncompressedOffset;
  bool lastMetaBlock;
  uint32_t paddingBits;

  PreflateParameters params;
  PreflateStatisticsCounter counter;
  std::unique_ptr<PreflateTokenPredictor> tokenPredictor;
  std::unique_ptr<PreflateTreePredictor> treePredictor;
};

bool preflate_decode(OutputStream& unpacked_output,
                     std::vector<unsigned char>& preflate_diff,
                     uint64_t& deflate_size,
                     InputStream& deflate_raw,
                     std::function<void(void)> block_callback,
                     const size_t min_deflate_size,
                     const size_t metaBlockSize = INT32_MAX);

bool preflate_decode(std::vector<unsigned char>& unpacked_output,
                     std::vector<unsigned char>& preflate_diff,
                     const std::vector<unsigned char>& deflate_raw,
                     const size_t metaBlockSize = INT32_MAX);

bool preflate_decode(std::vector<unsigned char>& unpacked_output,
                     std::vector<unsigned char>& preflate_diff,
                     uint64_t& deflate_size,
                     InputStream& deflate_raw,
                     std::function<void (void)> block_callback,
                     const size_t min_deflate_size,
                     const size_t metaBlockSize = INT32_MAX);

#endif /* PREFLATE_DECODER_H */
