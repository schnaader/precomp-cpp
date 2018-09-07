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
#include "outputcachestream.h"

OutputCacheStream::OutputCacheStream(OutputStream& os)
  : _os(os)
  , _cacheStartPos(0) {}
OutputCacheStream::~OutputCacheStream() {
}

void OutputCacheStream::flushUpTo(const uint64_t newStartPos) {
  size_t toWrite = std::min(newStartPos - _cacheStartPos, (uint64_t)_cache.size());
  size_t written = _os.write(_cache.data(), toWrite);
  _cacheStartPos += written;
  _cache.erase(_cache.begin(), _cache.begin() + written);
}
