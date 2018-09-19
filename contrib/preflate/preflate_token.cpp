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

#include "preflate_token.h"

bool isEqual(const PreflateTokenBlock& b1, const PreflateTokenBlock& b2) {
  if (b1.type != b2.type) {
    return false;
  }
//  if (b1.uncompressedLen != b2.uncompressedLen) {
//    return false;
//  }
  if (b1.type != PreflateTokenBlock::STORED) {
    if (b1.type == PreflateTokenBlock::DYNAMIC_HUFF) {
      if (b1.ncode != b2.ncode || b1.nlen != b2.nlen || b1.ndist != b2.ndist) {
        return false;
      }
      if (b1.treecodes != b2.treecodes) {
        return false;
      }
    }
    if (b1.tokens.size() != b2.tokens.size()) {
      return false;
    }
    for (unsigned i = 0, n = b1.tokens.size(); i < n; ++i) {
      if (b1.tokens[i].len != b2.tokens[i].len || b1.tokens[i].dist != b2.tokens[i].dist) {
        return false;
      }
    }
  }
  return true;
}
