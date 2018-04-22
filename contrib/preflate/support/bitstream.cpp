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
#include <memory.h>
#include "bitstream.h"

BitInputStream::BitInputStream(InputStream& is)
  : _input(is)
  , _bufPos(0)
  , _bufSize(0)
  , _bufFastLimit(0)
  , _eof(false)
  , _bits(0)
  , _bitsRemaining(0)
  , _totalBitPos(0)
{}

void BitInputStream::_fillBytes() {
  // free space in bit buffer
  if (_bufPos >= _bufFastLimit) {
    if (!_eof) {
      unsigned remaining = _bufSize - _bufPos;
      memcpy(_buffer + PRE_BUF_EXTRA - remaining,
             _buffer + _bufPos, remaining);
      _bufPos = PRE_BUF_EXTRA - remaining;
      _bufSize = PRE_BUF_EXTRA + _input.read(_buffer + PRE_BUF_EXTRA, BUF_SIZE);
      _bufFastLimit = std::max(_bufPos, _bufSize - PRE_BUF_EXTRA);
      _eof = _bufSize != PRE_BUF_EXTRA + BUF_SIZE;
    }
  }
}
void BitInputStream::_fill() {
  // free space in bit buffer
  if (_bufPos >= _bufFastLimit) {
    if (!_eof) {
      _fillBytes();
    }
    while (_bitsRemaining <= BITS - 8 && _bufPos < _bufSize) {
      _bits |= ((size_t)_buffer[_bufPos++]) << _bitsRemaining;
      _bitsRemaining += 8;
    }
    return;
  }
  while (_bitsRemaining <= BITS - 8) {
    _bits |= ((size_t)_buffer[_bufPos++]) << _bitsRemaining;
    _bitsRemaining += 8;
  }
}
size_t BitInputStream::copyBytesTo(OutputStream& output, const size_t len) {
  if (_bitsRemaining & 7) {
    return 0;
  }
  uint8_t a[sizeof(_bits)];
  size_t l = 0;
  while (_bitsRemaining > 0 && l < len) {
    a[l++] = _bits & 0xff;
    _bitsRemaining -= 8;
    _bits >>= 8;
    _totalBitPos += 8;
  }
  size_t w = output.write(a, l);
  if (w != l) {
    return w;
  }
  while (l < len) {
    unsigned todo = std::min(len - l, (size_t)(_bufSize - _bufPos));
    w = output.write(_buffer + _bufPos, todo);
    _totalBitPos += 8 * w;
    _bufPos += w;
    l += w;
    if (w != todo || eof()) {
      return l;
    }
    _fillBytes();
  }
  return l;
}
size_t BitInputStream::getBytes(uint8_t* data, const size_t size_) {
  skipToByte();
  size_t size = size_;
  while (_bitsRemaining > 0 && size > 0) {
    *data++ = _bits & 0xff;
    _bitsRemaining -= 8;
    _bits >>= 8;
    _totalBitPos += 8;
    size--;
  }
  while (size > 0) {
    unsigned todo = std::min(size, (size_t)(_bufSize - _bufPos));
    memcpy(data, _buffer + _bufPos, todo);
    data += todo;
    _totalBitPos += 8 * todo;
    _bufPos += todo;
    size -= todo;
    if (eof()) {
      return size_ - size;
    }
    _fillBytes();
  }
  return size_;
}
uint64_t BitInputStream::getVLI() {
  uint64_t result = 0, o = 0;
  unsigned s = 0, c;
  unsigned bitsRemaining = ((_bitsRemaining - 1) & 7) + 1;
  unsigned limit = 1 << (bitsRemaining - 1);
  while ((c = get(bitsRemaining)) >= limit) {
    result += ((uint64_t)(c & (limit - 1))) << s;
    s += (bitsRemaining - 1);
    o = (o + 1) << (bitsRemaining - 1);
    bitsRemaining = 8;
    limit = 128;
  }
  return result + o + (((uint64_t)c) << s);
}

BitOutputStream::BitOutputStream(OutputStream& output)
  : _output(output)
  , _bufPos(0)
  , _bits(0)
  , _bitPos(0) {}

void BitOutputStream::_flush() {
  while (_bitPos >= 8) {
    _buffer[_bufPos++] = _bits & 0xff;
    _bits >>= 8;
    _bitPos -= 8;
  }
  if (_bufPos >= BUF_SIZE) {
    _output.write(_buffer, BUF_SIZE);
    memcpy(_buffer, _buffer + BUF_SIZE, _bufPos - BUF_SIZE);
    _bufPos -= BUF_SIZE;
  }
}
void BitOutputStream::flush() {
  _flush();

  if (_bitPos > 0) {
    _buffer[_bufPos++] = _bits & 0xff;
    _bits   = 0;
    _bitPos = 0;
  }

  _output.write(_buffer, _bufPos);
  _bufPos = 0;
}
void BitOutputStream::putBytes(const uint8_t* data, const size_t size) {
  flush();
  _output.write(data, size);
}
void BitOutputStream::putVLI(const uint64_t size_) {
  uint64_t size = size_;
  unsigned bitsRemaining = 8 - (_bitPos & 7);
  unsigned limit = 1 << (bitsRemaining - 1);
  while (size >= limit) {
    put(size | limit, bitsRemaining);
    size = (size >> (bitsRemaining - 1)) - 1;
    bitsRemaining = 8;
    limit = 128;
  }
  put(size, bitsRemaining);
 
}
