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

#include <stdlib.h>
#include "preflate_dumper.h"
#include "preflate_unpack.h"
#include "zlib1.2.11.dec/zlib.h"

void* zalloc(void*, uInt items, uInt size) {
  return malloc(items * size);
}
void zfree(void*, void* addr) {
  free(addr);
}

bool preflate_unpack(std::vector<unsigned char>& unpacked_output,
                     std::vector<PreflateTokenBlock>& blocks,
                     const std::vector<unsigned char>& deflate_raw) {
  z_stream inf;      /* zlib deflate and inflate states */
  inf.zalloc = zalloc;
  inf.zfree = zfree;
  inf.opaque = Z_NULL;
  inf.avail_in = deflate_raw.size();
  inf.next_in = const_cast<unsigned char*>(deflate_raw.data());

  PreflateDumper dumper;
  inf.dumper = &dumper;
  int ret = inflateInit2(&inf, -15);
  if (ret != Z_OK) {
    return false;
  }
  std::vector<unsigned char> tmp(64 * 1024);
  inf.avail_out = tmp.size();
  inf.next_out = tmp.data();
  bool failure = false;

  do {
    /* decompress */
    inf.avail_out = tmp.size();
    inf.next_out = tmp.data();
    ret = inflate(&inf, Z_NO_FLUSH);
    if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_NEED_DICT || ret == Z_MEM_ERROR
        || ret == Z_BUF_ERROR) {
      return false;
    }
    dumper.uncompressed.insert(dumper.uncompressed.end(), tmp.begin(), tmp.end() - inf.avail_out);
  } while (ret != Z_STREAM_END);

  inflateEnd(&inf);

  if (failure || dumper.hadErrors()) {
    return false;
  }

  unpacked_output = std::move(dumper.uncompressed);
  blocks = std::move(dumper.blocks);
  return true;
}
