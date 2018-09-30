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

#ifndef OUTPUTCACHESTREAM_H
#define OUTPUTCACHESTREAM_H

#include <algorithm>
#include <vector>
#include "stream.h"

class OutputCacheStream : public OutputStream {
public:
  OutputCacheStream(OutputStream& os);
  virtual ~OutputCacheStream();

  size_t write(const unsigned char* buffer, const size_t size) {
/*    if (size == 1) {
      _cache.push_back(*buffer);
      return 1;
    }*/
    _cache.insert(_cache.end(), buffer, buffer + size);
    return size;
  }
  void reserve(const size_t len) {
    size_t cap = _cache.capacity();
    if (_cache.size() + len > cap) {
      _cache.reserve(cap + std::max(cap >> 1, len));
    }
  }
  void flush() {
    flushUpTo(cacheEndPos());
  }
  void flushUpTo(const uint64_t newStartPos);
  uint64_t cacheStartPos() const {
    return _cacheStartPos;
  }
  uint64_t cacheEndPos() const {
    return _cacheStartPos + _cache.size();
  }
  const unsigned char* cacheData(const uint64_t pos) const {
    return _cache.data() + (std::ptrdiff_t)(pos - _cacheStartPos);
  }
  const unsigned char* cacheEnd() const {
    return _cache.data() + _cache.size();
  }
  const size_t cacheSize() const {
    return _cache.size();
  }

private:
  OutputStream& _os;
  std::vector<unsigned char> _cache;
  uint64_t _cacheStartPos;
};

#endif /* OUTPUTCACHESTREAM_H */
