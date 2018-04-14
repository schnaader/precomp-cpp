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

#ifndef FILESTREAM_H
#define FILESTREAM_H

#include <stdint.h>
#include <vector>
#include "stream.h"

class FileStream : public SeekableInputOutputStream {
public:
  FileStream(FILE* f);

  virtual bool eof() const;
  virtual size_t read(unsigned char* buffer, const size_t size);

  virtual size_t write(const unsigned char* buffer, const size_t size);

  virtual uint64_t tell() const;
  virtual uint64_t seek(const uint64_t newPos);

private:
  FILE* _f;
};

#endif /* FILESTREAM_H */
