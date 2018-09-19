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

#ifndef HUFFMAN_HELPER_H
#define HUFFMAN_HELPER_H

#include <vector>

// Huffman decoder
class HuffmanHelper {
public:
  enum {
    MAX_BL = 25
  };
  static bool countSymbols(unsigned(&nextCode)[MAX_BL + 2],
                           unsigned char& minLength,
                           unsigned char& maxLength,
                           const unsigned char* symbolBitLengths,
                           const unsigned symbolCount,
                           const bool disableZeroBitSymbols);
};

#endif /* HUFFMAN_HELPER_H */
