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

#ifndef HUFFMAN_DECODER_H
#define HUFFMAN_DECODER_H

#include <vector>
#include "bitstream.h"

// Huffman decoder
class HuffmanDecoder {
public:
  HuffmanDecoder(const unsigned char* symbolBitLengths,
                 const size_t symbolCount,
                 const bool disableZeroBitSymbols,
                 const unsigned char maxBitsPerTable);

  bool error() const {
    return _error;
  }

  size_t decode(BitInputStream& bis) const {
    size_t v = bis.peek(_table0.peekBits);
    signed short w = _table0.lookup[v];
    if (w >= 0) {
      bis.skip(w & 0xf);
      return w >> 4;
    }
    return _decodeDeeper(bis, ~w);
  }

private:
  size_t _decodeDeeper(BitInputStream& bis, const size_t tableId) const;
  bool _constructTables(const unsigned char* symbolBitLengths,
                        const size_t symbolCount,
                        const bool disableZeroBitSymbols,
                        const unsigned char maxBitsPerTable);
  void _constructErrorTable();

private:
  struct Table {
    unsigned char peekBits;
    std::vector<signed short> lookup;
  };

  Table _table0;
  std::vector<Table> _tables;
  bool _error;
};

#endif /* HUFFMAN_DECODER_H */
