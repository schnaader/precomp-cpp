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
#include "memstream.h"

MemStream::MemStream() : _pos(0) {}
MemStream::MemStream(const std::vector<uint8_t>& content)
  : _data(content)
  , _pos(0) {}
MemStream::MemStream(const std::vector<uint8_t>& content, const size_t off, const size_t sz)
  : _data(std::max(std::min(content.size(), off + sz), off) - off)
  , _pos(0) {
  memcpy(_data.data(), content.data() + off, _data.size());
}

bool MemStream::eof() const {
  return _pos == _data.size();
}
size_t MemStream::read(unsigned char* buffer, const size_t size) {
  size_t toCopy = std::min(size, _data.size() - _pos);
  memcpy(buffer, _data.data() + _pos, toCopy);
  _pos += toCopy;
  return toCopy;
}

size_t MemStream::write(const unsigned char* buffer, const size_t size) {
  size_t remaining = _data.size() - _pos;
  if (size > remaining) {
    _data.resize(_pos + size);
  }
  memcpy(_data.data() + _pos, buffer, size);
  _pos += size;
  return size;
}

uint64_t MemStream::tell() const {
  return _pos;
}
uint64_t MemStream::seek(const uint64_t newPos) {
  size_t oldPos = _pos;
  _pos = std::min(newPos, (uint64_t)_data.size());
  return oldPos;
}
