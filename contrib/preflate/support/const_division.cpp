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
#include "const_division.h"

// Based on "N-Bit Unsigned Division Via N-Bit Multiply-Add"
// by Robison

template <unsigned N>
udivider_t<N> build_udivider(const typename divider_uint_t<N>::type d) {
  typedef typename divider_uint_t<N * 2>::type T1;
  typedef typename divider_uint_t<N>::type T2;
  udivider_t<N> result;
  result.shift = bitLength(d) - 1;
  if ((d & (d - 1)) == 0) {
    result.magic1 = result.magic2 = ~(T2)0;
  } else {
    T2 shm = 1 << result.shift;
    T2 t = (((T1)shm) << N) / d;
    T2 r = t * d + d;
    if (r <= shm) {
      result.magic1 = t + 1;
      result.magic2 = 0;
    } else {
      result.magic1 = t;
      result.magic2 = t;
    }
  }
  return result;
}

udivider_t<16> build_udivider_16(const uint16_t d) {
  return build_udivider<16>(d);
}
udivider_t<32> build_udivider_32(const uint32_t d) {
  return build_udivider<32>(d);
}

template <unsigned N>
ucdivider_t<N> build_ucdivider(const typename divider_uint_t<N>::type d) {
  typedef typename divider_uint_t<N * 2>::type T1;
  typedef typename divider_uint_t<N>::type T2;
  ucdivider_t<N> result;
  result.ctrl = bitLength(d) - 1;
  if ((d & (d - 1)) == 0) {
    result.magic = ~(T2)0;
    result.ctrl |= 0x80;
  } else {
    T2 shm = 1 << result.ctrl;
    T2 t = (((T1)shm) << N) / d;
    T2 r = t * d + d;
    if (r <= shm) {
      result.magic = t + 1;
    } else {
      result.magic = t;
      result.ctrl |= 0x80;
    }
  }
  return result;
}

ucdivider_t<16> build_ucdivider_16(const uint16_t d) {
  return build_ucdivider<16>(d);
}
ucdivider_t<32> build_ucdivider_32(const uint32_t d) {
  return build_ucdivider<32>(d);
}

template <unsigned N>
sdivider_t<N> build_sdivider(const typename divider_int_t<N>::type d_) {
  sdivider_t<N> result;
  udivider_t<N> uresult = build_udivider<N>(d_ < 0 ? -d_ : d_);
  result.magic1 = uresult.magic1;
  result.magic2 = uresult.magic2;
  result.shift = uresult.shift;
  result.sign = d_ < 0 ? -1 : 0;
  return result;
}
sdivider_t<16> build_sdivider_16(const int16_t d) {
  return build_sdivider<16>(d);
}
sdivider_t<32> build_sdivider_32(const int32_t d) {
  return build_sdivider<32>(d);
}

template <unsigned N>
scdivider_t<N> build_scdivider(const typename divider_int_t<N>::type d_) {
  scdivider_t<N> result;
  ucdivider_t<N> uresult = build_ucdivider<N>(d_ < 0 ? -d_ : d_);
  result.magic = uresult.magic;
  result.ctrl  = uresult.ctrl;
  if (d_ < 0) {
    result.ctrl |= 0x40;
  }
  return result;
}
scdivider_t<16> build_scdivider_16(const int16_t d) {
  return build_scdivider<16>(d);
}
scdivider_t<32> build_scdivider_32(const int32_t d) {
  return build_scdivider<32>(d);
}
