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
#include "huffman_helper.h"
#include "bit_helper.h"

bool HuffmanHelper::countSymbols(
    unsigned(&nextCode)[MAX_BL + 2],
    unsigned char& minLength,
    unsigned char& maxLength,
    const unsigned char* symbolBitLengths,
    const unsigned symbolCount,
    const bool disableZeroBitSymbols
) {
  if (symbolCount < 1 || symbolCount >= 1024) {
    return false;
  }
  unsigned short blCount[MAX_BL + 2];

  // Count symbol frequencies
  memset(blCount, 0, sizeof(blCount));
  for (unsigned i = 0; i < symbolCount; ++i) {
    unsigned char l = (unsigned char)(symbolBitLengths[i] + 1);
    if (l > MAX_BL + 1) {
      return false;
    }
    blCount[l]++;
  }
  for (minLength = 1; minLength <= MAX_BL + 1; ++minLength) {
    if (blCount[minLength]) {
      break;
    }
  }
  for (maxLength = MAX_BL + 1; maxLength >= minLength; --maxLength) {
    if (blCount[maxLength]) {
      break;
    }
  }
  if (minLength > maxLength) {
    return false;
  }
  // Remove deleted symbols
  blCount[0] = 0;
  if (disableZeroBitSymbols) {
    blCount[1] = 0;
  }

  // Calculate start codes
  unsigned code = 0;
  for (unsigned i = minLength; i <= maxLength; ++i) {
    code = (code + blCount[i - 1]) << 1;
    nextCode[i] = code;
  }

  if (minLength == maxLength && blCount[maxLength] == 1) {
    return true;
  }

  // Check that we don't have holes
  unsigned codeCheck = nextCode[maxLength] + blCount[maxLength];
  return codeCheck == (unsigned)(1 << (maxLength - 1)) || codeCheck == 1 && maxLength == 2;
}
