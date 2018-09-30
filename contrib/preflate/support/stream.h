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

#ifndef STREAM_H
#define STREAM_H

#include <stdint.h>

class InputStream {
public:
  virtual ~InputStream() {}

  virtual bool eof() const = 0;
  virtual size_t read(unsigned char* buffer, const size_t size) = 0;
};

class OutputStream {
public:
  virtual ~OutputStream() {}

  virtual size_t write(const unsigned char* buffer, const size_t size) = 0;
};

class SeekableStream {
public:
  virtual ~SeekableStream() {}

  virtual uint64_t tell() const = 0;
  virtual uint64_t seek(const uint64_t newPos) = 0;
};

class SeekableInputStream 
  : public InputStream
  , public SeekableStream {};
class SeekableInputOutputStream 
  : public SeekableInputStream
  , public OutputStream {};


#endif /* STREAM_H */
