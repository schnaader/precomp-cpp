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

#ifndef HUFFMAN_ENCODER_H
#define HUFFMAN_ENCODER_H

#include <vector>
#include "bitstream.h"

// Huffman decoder
class HuffmanEncoder {
public:
  HuffmanEncoder(const unsigned char* symbolBitLengths,
                 const unsigned symbolCount,
                 const bool disableZeroBitSymbols);

  bool error() const {
    return _error;
  }

  void encode(BitOutputStream& bos, const unsigned symbol) const {
    unsigned v = _lookup[symbol];
    bos.put(v >> 5, v & 0x1f);
  }

private:
  bool _constructTables(const unsigned char* symbolBitLengths,
                        const unsigned symbolCount,
                        const bool disableZeroBitSymbols);
  void _constructErrorTable(const unsigned symbolCount);

private:
  std::vector<unsigned> _lookup;
  bool _error;
};

#endif /* HUFFMAN_ENCODER_H */
