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

#include <stdio.h>
#include "array_helper.h"
#include "bit_helper.h"
#include "bitstream.h"
#include "const_division.h"
#include "huffman_decoder.h"
#include "huffman_encoder.h"
#include "huffman_helper.h"
#include "memstream.h"
#include "outputcachestream.h"
#include "stream.h"

bool support_self_tests() {
  unsigned arr[] = {1,2,3,4,5};
  if (sumArray(arr) != 15
      || sumArray(arr, sizeof(arr) / sizeof(arr[0])) != 15) {
    printf("sumArray failed\n");
    return false;
  }
  if (bitLength(0) != 0 
      || bitLength(15) != 4
      || bitLength(0xffffffff) != 32) {
    printf("bitLength failed\n");
    return false;
  }
  if (bitReverse(1, 3) != 4
      || bitReverse(0x12345678, 32) != 0x1e6a2c48
      || bitReverse(0xfedcba90, 32) != 0x095d3b7f) {
    printf("bitReverse failed\n");
    return false;
  }

  MemStream mem;
  mem.write((const uint8_t*)"Hello", 5);
  if (mem.tell() != 5 || !mem.eof()) {
    printf("MemStream/1 failed\n");
    return false;
  }
  mem.write((const uint8_t*)"!", 1);
  uint8_t tmp[5], tmp2[2];
  if (mem.read(tmp, 5) != 0) {
    printf("MemStream/2 failed\n");
    return false;
  }
  if (mem.seek(0) != 6) {
    printf("MemStream/3 failed\n");
    return false;
  }
  if (mem.tell() != 0) {
    printf("MemStream/4 failed\n");
    return false;
  }
  if (mem.read(tmp, 5) != 5 || tmp[0] != 'H' || tmp[4] != 'o') {
    printf("MemStream/5 failed\n");
    return false;
  }
  if (mem.read(tmp2, 2) != 1 || tmp2[0] != '!') {
    printf("MemStream/6 failed\n");
    return false;
  }
  if (!mem.eof()) {
    printf("MemStream/7 failed\n");
    return false;
  }

  mem.seek(0);
  {
    BitOutputStream bos(mem);
    for (unsigned i = 0; i <= HuffmanHelper::MAX_BL; ++i) {
      bos.put(i, i);
    }
    bos.flush();
  }
  mem.seek(0);
  {
    BitInputStream bis(mem);
    for (unsigned i = 0; i <= HuffmanHelper::MAX_BL; ++i) {
      if (bis.get(i) != i) {
       printf("BitStreams failed\n");
       return false;
      }
    }
  }

  unsigned char lengths[] = {
    1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
    17,18,19,20,21,22,23,24,25,25
  };
  unsigned count = sizeof(lengths) / sizeof(lengths[0]);
  HuffmanEncoder henc(lengths, count, false);
  HuffmanDecoder hdec(lengths, count, false, 7);
  if (henc.error() || hdec.error()) {
    printf("HuffmanEncoder failed\n");
    return false;
  }
  mem.seek(0);
  {
    BitOutputStream bos(mem);
    for (unsigned i = 0; i < count; ++i) {
      henc.encode(bos, i);
    }
    bos.flush();
  }
  mem.seek(0);
  {
    BitInputStream bis(mem);
    for (unsigned i = 0; i < count; ++i) {
      if (hdec.decode(bis) != i) {
        printf("HuffmanDecoder failed\n");
        return false;
      }
    }
  }

  uint16_t divtest16[] = {1, 3, 5, 7, 9, 11, 13, 17, 32767};
  for (int i = 0, n = sizeof(divtest16) / sizeof(divtest16[0]); i < n; ++i) {
    udivider_t<16> du   = build_udivider_16(divtest16[i]);
    ucdivider_t<16> duc = build_ucdivider_16(divtest16[i]);
    sdivider_t<16> ds = build_sdivider_16(divtest16[i]);
    scdivider_t<16> dsc = build_scdivider_16(divtest16[i]);

    for (int k = 0; k < 65536; ++k) {
      uint16_t c1 = divide((uint16_t)k, du);
      uint16_t c2 = divide((uint16_t)k, duc);
      uint16_t r = k / divtest16[i];
      if (c1 != r || c2 != r) {
        printf("16bit divider/1 failed\n");
        return false;
      }

      int16_t d1 = divide((int16_t)(k - 32768), ds);
      int16_t d2 = divide((int16_t)(k - 32768), dsc);
      int16_t s = ((int16_t)(k - 32768)) / (int16_t)divtest16[i];
      if (d1 != s || d2 != s) {
        printf("16bit divider/2 failed\n");
        return false;
      }
    }
  }
  uint32_t divtest32[] = {1, 3, 5, 7, 9, 11, 13, 17, 0x7fff, 0x7fffffff};
  for (int i = 0, n = sizeof(divtest32) / sizeof(divtest32[0]); i < n; ++i) {
    udivider_t<32> du = build_udivider_32(divtest32[i]);
    ucdivider_t<32> duc = build_ucdivider_32(divtest32[i]);
    sdivider_t<32> ds = build_sdivider_32(divtest32[i]);
    scdivider_t<32> dsc = build_scdivider_32(divtest32[i]);

    for (int k = 0; k < 65536; ++k) {
      uint32_t c1 = divide(((uint32_t)k)* 65536, du);
      uint32_t c2 = divide(((uint32_t)k) * 65536, duc);
      uint32_t r = (((uint32_t)k) * 65536) / divtest32[i];
      if (c1 != r || c2 != r) {
        printf("32bit divider/1 failed\n");
        return false;
      }

      int32_t d1 = divide((int32_t)(k - 32768) * 65536, ds);
      int32_t d2 = divide((int32_t)(k - 32768) * 65536, dsc);
      int32_t s = ((int32_t)(k - 32768)) * 65536 / (int32_t)divtest32[i];
      if (d1 != s || d2 != s) {
        printf("32bit divider/2 failed\n");
        return false;
      }
    }
  }
  return true;
}
