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

#include "bit_helper.h"

unsigned bitLength(unsigned value) {
  unsigned l = 0;
  while (value > 0) {
    l++;
    value >>= 1;
  }
  return l;
}

static unsigned char reverse4[16] = {0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15};
static unsigned bitReverse8(const unsigned value) {
  return (reverse4[value & 0x0f] << 4) | reverse4[(value >> 4) & 0x0f];
}
static unsigned bitReverse16(const unsigned value) {
  return (bitReverse8(value & 0xff) << 8) | bitReverse8(value >> 8);
}
static unsigned bitReverse32(const unsigned value) {
  return (bitReverse16(value & 0xffff) << 16) | bitReverse16(value >> 16);
}
unsigned bitReverse(const unsigned value, const unsigned bits) {
  if (bits <= 8) {
    return bitReverse8(value) >> (8 - bits);
  }
  if (bits <= 16) {
    return bitReverse16(value) >> (16 - bits);
  }
  return bitReverse32(value) >> (32 - bits);
}

static unsigned char leading4[16] = {4, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned bitLeadingZeroes(const unsigned value_) {
  if (value_ == 0) {
    return 32;
  }
  unsigned value = value_;
  unsigned result = 0;
  while ((value & 0xf0000000) == 0) {
    value <<= 4;
    result += 4;
  }
  return result + leading4[value >> 28];
}
static unsigned char trailing4[16] = {4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0};
unsigned bitTrailingZeroes(const unsigned value_) {
  if (value_ == 0) {
    return 32;
  }
  unsigned value = value_;
  unsigned result = 0;
  while ((value & 0xf) == 0) {
    value >>= 4;
    result += 4;
  }
  return result + trailing4[value & 0xf];
}

