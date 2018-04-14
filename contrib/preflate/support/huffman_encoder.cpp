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
#include "huffman_encoder.h"
#include "huffman_helper.h"
#include "bit_helper.h"

HuffmanEncoder::HuffmanEncoder(
    const unsigned char* symbolBitLengths,
    const unsigned symbolCount,
    const bool disableZeroBitSymbols
) : _error(false) {
  if (!_constructTables(symbolBitLengths, symbolCount, disableZeroBitSymbols)) {
    _constructErrorTable(symbolCount);
  }
}

bool HuffmanEncoder::_constructTables(
    const unsigned char* symbolBitLengths,
    const unsigned symbolCount,
    const bool disableZeroBitSymbols
) {
  unsigned nextCode[HuffmanHelper::MAX_BL + 2];
  unsigned char minLength, maxLength;
  if (!HuffmanHelper::countSymbols(nextCode, minLength, maxLength,
                                   symbolBitLengths, symbolCount,
                                   disableZeroBitSymbols)) {
    return false;
  }

  unsigned char minL = disableZeroBitSymbols ? 2 : 1;

  _lookup.resize(symbolCount);
  for (unsigned i = 0; i < symbolCount; ++i) {
    unsigned char l = (unsigned char)(symbolBitLengths[i] + 1);
    if (l < minL) {
      _lookup[i] = 0;
      continue;
    }
    unsigned char k = l - 1;
    unsigned code = bitReverse(nextCode[l]++, k);
    _lookup[i] = (code << 5) | k;
  }
  return true;
}
void HuffmanEncoder::_constructErrorTable(
    const unsigned symbolCount
) {
  _error = true;
  _lookup.resize(symbolCount);
  std::fill(_lookup.begin(), _lookup.end(), 0);
}
