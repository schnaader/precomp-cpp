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
#include "preflate_constants.h"
#include "preflate_seq_chain.h"

PreflateSeqChain::PreflateSeqChain(
    const std::vector<unsigned char>& input_)
  : _input(input_)
  , totalShift(-8)
  , curPos(0) {
  prev = new SeqChainEntry[1 << 16];
  memset(heads, 0x00, sizeof(heads));
  _build(8, std::min<uint32_t>((1 << 16) - 8, _input.remaining()));
}
PreflateSeqChain::~PreflateSeqChain() {
  delete[] prev;
}

void PreflateSeqChain::_reshift() {
  const unsigned short delta = 0x7e00;
  unsigned remaining = (1 << 16) - (delta + 8);
  // If the head of large sequence is shifted out,
  // but the tail remains in the cache, 
  // we need to adapt the head and all pointers to it,
  // that is all members, the next non-member pointing to it
  // or heads
  if (prev[delta + 8].distToNext != 0xffff && prev[delta + 8].length < PreflateConstants::MIN_MATCH) {
    unsigned d = prev[delta + 8].distToNext;
    prev[delta + 8].distToNext = 0xffff;
    prev[delta + 8].length = prev[delta + 8 - d].length - d;
    for (unsigned i = 3; i < prev[delta + 8].length; ++i) {
      prev[delta + 8 + i - 2].distToNext -= d;
    }
    uint8_t c = *_input.curChars(-(int)remaining);
    if (heads[c] == delta + 8 - d) { 
      heads[c] += d;
    } else {
      for (unsigned i = prev[delta + 8].length; i < remaining;  ++i) {
        if (prev[delta + 8 + i].distToNext == i + d) {
          prev[delta + 8 + i].distToNext -= d;
          break;
        }
      }
    }
  }
  for (unsigned i = 0; i < 256; ++i) {
    heads[i] = std::max(heads[i], delta) - delta;
  }
  memmove(prev + 8, prev + (delta + 8), sizeof(SeqChainEntry) * remaining);
  totalShift += delta;
  _build(8 + remaining, std::min<uint32_t>(delta, _input.remaining()));
}
void PreflateSeqChain::_build(const unsigned off0, const unsigned size) {
  if (!size) {
    return;
  }
  const unsigned char* b = _input.curChars();
  uint8_t curChar = b[0];
  SeqChainEntry startOfSeq = {0xffff, 0x0}, *ptrToFirstOfSeq;
  unsigned startOff = off0;
  prev[off0] = startOfSeq;
  if (off0 > 8 && curChar == b[-1]) {
    --startOff;
    // new block continues the old
    if (curChar == b[-2]) {
      --startOff;
      // this is definitely a sequence
      if (curChar == b[-3]) {
        // This was already a sequence in the previous block,
        // just append
        startOff = heads[curChar];
        prev[off0 - 2].distToNext = off0 - startOff - 2;
        prev[off0 - 1].distToNext = off0 - startOff - 1;
        prev[off0].distToNext = off0 - startOff;
        prev[off0].length = 1;
      } else {
        // Otherwise enter the sequence in the books
        prev[startOff].distToNext = startOff - heads[curChar];
        prev[startOff + 1].distToNext = 1;
        prev[startOff + 2].distToNext = 2;
        prev[startOff + 2].length = 1;
        heads[curChar] = startOff;
      }
    } else {
      prev[startOff + 1].distToNext = 1;
      prev[startOff + 1].length = 1;
    }
  }
  ptrToFirstOfSeq = &prev[startOff];
  ++ptrToFirstOfSeq->length;

  uint8_t prevChar = curChar;
  for (unsigned i = 1; i < size; ++i) {
    curChar = b[i];
    if (prevChar == curChar) {
      if (++ptrToFirstOfSeq->length == 3) {
        prev[startOff].distToNext = startOff - heads[prevChar];
        heads[prevChar] = startOff;
      }
      prev[off0 + i].distToNext = off0 + i - startOff;
      prev[off0 + i].length = 1;
    } else {
      // Last two of a sequence are not a sequence themselves
      if (ptrToFirstOfSeq->length >= 2) {
        if (ptrToFirstOfSeq->length >= 3) {
          prev[off0 + i - 2].distToNext = 0xffff;
        }
        prev[off0 + i - 1].distToNext = 0xffff;
      }
      prev[off0 + i] = startOfSeq;
      startOff = off0 + i;
      ptrToFirstOfSeq = &prev[startOff];
      ++ptrToFirstOfSeq->length;
    }
    prevChar = curChar;
  }
  // Last two of a sequence are not a sequence themselves
  if (ptrToFirstOfSeq->length >= 2) {
    if (ptrToFirstOfSeq->length >= 3) {
      prev[off0 + size - 2].distToNext = 0xffff;
    }
    prev[off0 + size - 1].distToNext = 0xffff;
  }
  _input.advance(size);
}
