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
#include "huffman_decoder.h"
#include "huffman_helper.h"
#include "bit_helper.h"

HuffmanDecoder::HuffmanDecoder(
    const unsigned char* symbolBitLengths,
    const size_t symbolCount,
    const bool disableZeroBitSymbols,
    const unsigned char maxBitsPerTable
) : _error(false) {
  if (!_constructTables(symbolBitLengths, symbolCount, disableZeroBitSymbols, maxBitsPerTable)) {
    _constructErrorTable();
  }
}

size_t HuffmanDecoder::_decodeDeeper(
    BitInputStream& bis, 
    const size_t tableId_
) const {
  bis.skip(_table0.peekBits);
  size_t tableId = tableId_;
  do {
    const Table* table = &_tables[tableId];
    size_t v = bis.peek(table->peekBits);
    signed short w = table->lookup[v];
    if (w >= 0) {
      bis.skip(w & 0xf);
      return w >> 4;
    }
    bis.skip(table->peekBits);
    tableId = ~w;
  } while (true);
}
bool HuffmanDecoder::_constructTables(
    const unsigned char* symbolBitLengths,
    const size_t symbolCount,
    const bool disableZeroBitSymbols,
    const unsigned char maxBitsPerTable
) {
  if (maxBitsPerTable < 1 || maxBitsPerTable > 15) {
    return false;
  }
  unsigned nextCode[HuffmanHelper::MAX_BL + 2];
  unsigned char minLength, maxLength;
  if (!HuffmanHelper::countSymbols(nextCode, minLength, maxLength,
                                   symbolBitLengths, symbolCount, 
                                   disableZeroBitSymbols)) {
    return false;
  }

  _table0.peekBits = std::min((unsigned char)(maxLength - 1), maxBitsPerTable);
  _table0.lookup.resize(1 << _table0.peekBits);
  std::fill(_table0.lookup.begin(), _table0.lookup.end(), 0);

  unsigned char minL = disableZeroBitSymbols ? 2 : 1;

  for (unsigned i = 0; i < symbolCount; ++i) {
    unsigned char l = (unsigned char)(symbolBitLengths[i] + 1);
    if (l < minL) {
      continue;
    }
    unsigned char k = l - 1, maxK = maxLength - 1;
    unsigned code = bitReverse(nextCode[l]++, k);
    Table* t = &_table0;
    while (k > t->peekBits) {
      k -= t->peekBits;
      maxK -= t->peekBits;
      unsigned subbits = code & ((1 << t->peekBits) - 1);
      code >>= t->peekBits;
      signed short v = t->lookup[subbits];
      if (v >= 0) {
        unsigned newTableId = _tables.size();
        t->lookup[subbits] = ~newTableId;
        _tables.push_back(Table());
        t = &_tables[newTableId];
        t->peekBits = std::min(maxK, maxBitsPerTable);
        t->lookup.resize(1 << t->peekBits);
        std::fill(t->lookup.begin(), t->lookup.end(), 0);
      } else {
        t = &_tables[~v];
      }
    }
    do {
      t->lookup[code] = (i << 4) | k;
      code += 1 << k;
    } while (code < t->lookup.size());
  }
  return true;
}
void HuffmanDecoder::_constructErrorTable() {
  _error = true;
  _table0.peekBits = 0;
  _table0.lookup.resize(1);
  _table0.lookup[0] = 0;
}
