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

#include <algorithm>
#include <string.h>
#include "preflate_info.h"

// -----------------------------------------

PreflateStreamInfo extractPreflateInfo(const std::vector<PreflateTokenBlock>& blocks) {
  PreflateStreamInfo result;
  memset(&result, 0, sizeof(result));
  result.countBlocks = blocks.size();
  for (unsigned i = 0, n = result.countBlocks; i < n; ++i) {
    const PreflateTokenBlock& b = blocks[i];
    if (b.type == PreflateTokenBlock::STORED) {
      result.countStoredBlocks++;
      continue;
    }
    if (b.type == PreflateTokenBlock::STATIC_HUFF) {
      result.countStaticHuffTreeBlocks++;
    }
    result.tokenCount += b.tokens.size();
    result.maxTokensPerBlock = std::max(result.maxTokensPerBlock, (unsigned)b.tokens.size());
    unsigned blockMaxDist = 0;
    for (unsigned j = 0, m = b.tokens.size(); j < m; ++j) {
      const PreflateToken& t = b.tokens[j];
      if (t.len == 1) {
        result.literalCount++;
      } else {
        result.referenceCount++;
        blockMaxDist = std::max(blockMaxDist, (unsigned)t.dist);
      }
    }
    result.maxDist = std::max(result.maxDist, blockMaxDist);
    if (blockMaxDist == 0) {
      result.countHuffBlocks++;
    } else if (blockMaxDist == 1) {
      result.countRLEBlocks++;
    }
  }
  return result;
}

