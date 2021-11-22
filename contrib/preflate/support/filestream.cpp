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
#include <stdio.h>
#include "filestream.h"

FileStream::FileStream(FILE* f) : _f(f) {}

bool FileStream::eof() const {
  return feof(_f);
}
size_t FileStream::read(unsigned char* buffer, const size_t size) {
  return fread(buffer, 1, size, _f);
}

size_t FileStream::write(const unsigned char* buffer, const size_t size) {
  return fwrite(buffer, 1, size, _f);
}

uint64_t FileStream::tell() const {
  #ifndef __unix
    return _ftelli64(_f);
  #else
    return ftello(_f);
  #endif
}
uint64_t FileStream::seek(const uint64_t newPos) {
  uint64_t oldPos = tell();
  #ifndef __unix
  _fseeki64(_f, newPos, SEEK_SET);
  #else
  fseeko(_f, newPos, SEEK_SET);
  #endif
  return oldPos;
}
