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

#ifndef PREFLATE_INFO_H
#define PREFLATE_INFO_H

#include "preflate_token.h"

struct PreflateStreamInfo {
  unsigned tokenCount;
  unsigned literalCount;
  unsigned referenceCount;
  unsigned maxDist;
  unsigned maxTokensPerBlock;
  unsigned countBlocks;
  unsigned countStoredBlocks;
  unsigned countHuffBlocks;
  unsigned countRLEBlocks;
  unsigned countStaticHuffTreeBlocks;
};

PreflateStreamInfo extractPreflateInfo(const std::vector<PreflateTokenBlock>& blocks);

#endif /* PREFLATE_INFO_H */