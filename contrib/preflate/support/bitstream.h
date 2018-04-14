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

#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <algorithm>
#include "bit_helper.h"
#include "stream.h"

// Huffman decoder for little endian 
class BitInputStream {
public:
  BitInputStream(InputStream&);

  bool eof() const {
    return _eof && _bufPos == _bufSize && !_bitsRemaining;
  }

  size_t bitPos() const {
    return _totalBitPos;
  }

  size_t peek(const unsigned n) {
    if (_bitsRemaining < n) {
      _fill();
    }
    return _bits & ((1 << n) - 1);
  }
  void skip(const unsigned n) {
    _bitsRemaining -= std::min(n, _bitsRemaining);
    _bits >>= n;
    _totalBitPos += n;
  }
  size_t get(const unsigned n) {
    size_t v = peek(n);
    skip(n);
    return v;
  }
  size_t getReverse(const unsigned n) {
    return bitReverse(get(n), n);
  }
  void skipToByte() {
    skip(_bitsRemaining & 7);
  }
  bool checkLastBitsOfByteAreZero() {
    return peek(_bitsRemaining & 7) == 0;
  }
  void fastFill(const unsigned n) {
    if (_bitsRemaining < n) {
      _fill();
    }
  }
  size_t fastPeek(const unsigned n) {
    return _bits & ((1 << n) - 1);
  }
  size_t fastGet(const unsigned n) {
    size_t v = fastPeek(n);
    skip(n);
    return v;
  }
  size_t copyBytesTo(OutputStream& output, const size_t len);

private:
  void _fillBytes();
  void _fill();

  enum { BUF_SIZE = 1024, PRE_BUF_EXTRA = 16, BITS = sizeof(size_t)*8 };

  InputStream& _input;
  unsigned char _buffer[PRE_BUF_EXTRA + BUF_SIZE];
  unsigned _bufPos, _bufSize, _bufFastLimit;
  bool _eof;
  size_t _bits;
  unsigned _bitsRemaining;
  size_t _totalBitPos;
};

class BitOutputStream {
public:
  BitOutputStream(OutputStream&);

  void put(const size_t value, const unsigned n) {
    if (_bitPos + n > BITS) {
      _flush();
    }
    _bits   |= (value & ((1 << n) - 1)) << _bitPos;
    _bitPos += n;
  }
  void putReverse(const size_t value, const unsigned n) {
    put(bitReverse(value, n), n);
  }
  void fillByte() {
    _bitPos = (_bitPos + 7) & ~7;
  }
  void flush();
  unsigned bitPos() const {
    return _bitPos;
  }

private:
  void _flush();

  enum {
    BUF_SIZE = 1024, BUF_EXTRA = 64, BITS = sizeof(size_t) * 8
  };

  OutputStream& _output;
  unsigned char _buffer[BUF_SIZE + BUF_EXTRA];
  unsigned _bufPos;
  size_t _bits;
  unsigned _bitPos;
};

#endif /* BITSTREAM_H */
