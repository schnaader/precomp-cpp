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

#ifndef CONST_DIVISION_H
#define CONST_DIVISION_H

#include <stdint.h>

template <unsigned N> struct divider_int_t;
template <unsigned N> struct divider_uint_t;
template <> struct divider_int_t<16> {
  typedef int16_t type; 
};
template <> struct divider_int_t<32> {
  typedef int32_t type;
};
template <> struct divider_uint_t<16> {
  typedef uint16_t type;
};
template <> struct divider_uint_t<32> {
  typedef uint32_t type;
};
template <> struct divider_uint_t<64> {
  typedef uint64_t type;
};


template <unsigned N>
struct udivider_t {
  typename divider_uint_t<N>::type magic1; // factor
  typename divider_uint_t<N>::type magic2; // addend
  uint8_t shift;
};

template <unsigned N>
struct ucdivider_t {
  typename divider_uint_t<N>::type magic; // factor/addend
  uint8_t ctrl; // bits 0..3/4/5 - shift, bit 7 - add required
};

// If it wasn't for +/-1, the signed dividers wouldn't
// need the add-term (magic2), as they could just
// use a factor (magic1) with one more bit precision.
template <unsigned N>
struct sdivider_t {
  typename divider_uint_t<N>::type magic1; // factor
  typename divider_uint_t<N>::type magic2; // addend
  uint8_t shift; 
  int8_t sign; // -1 if negative, 0 otherwise
};

template <unsigned N>
struct scdivider_t {
  typename divider_uint_t<N>::type magic;
  uint8_t ctrl; // bits 0..3/4/5 - shift, bit 6 - negative, bit 7 - add required
};

udivider_t<16> build_udivider_16(const uint16_t d);
udivider_t<32> build_udivider_32(const uint32_t d);

ucdivider_t<16> build_ucdivider_16(const uint16_t d);
ucdivider_t<32> build_ucdivider_32(const uint32_t d);

sdivider_t<16> build_sdivider_16(const int16_t d);
sdivider_t<32> build_sdivider_32(const int32_t d);

scdivider_t<16> build_scdivider_16(const int16_t d);
scdivider_t<32> build_scdivider_32(const int32_t d);

template <unsigned N1, unsigned N2>
inline typename divider_uint_t<N1>::type 
divide_template(const typename divider_uint_t<N1>::type dividend, 
                const udivider_t<N2>& divisor) {
  typedef typename divider_uint_t<N1 * 2>::type T1;
  typedef typename divider_uint_t<N1>::type T2;
  T1 t = ((T1)dividend) * divisor.magic1 + divisor.magic2;
  T2 u = (T2)(t >> N2);
  return u >> divisor.shift;
}
template <unsigned N1, unsigned N2>
inline typename divider_uint_t<N1>::type
divide_template(const typename divider_uint_t<N1>::type dividend,
                const ucdivider_t<N2>& divisor) {
  typedef typename divider_uint_t<N1 * 2>::type T1;
  typedef typename divider_uint_t<N1>::type T2;
  T1 t = ((T1)dividend) * divisor.magic
        + (divisor.ctrl & 0x80 ? divisor.magic : 0);
  T2 u = (T2)(t >> N2);
  return u >> (divisor.ctrl & (N2 - 1));
}
template <unsigned N1, unsigned N2>
inline typename divider_int_t<N1>::type
divide_template(const typename divider_int_t<N1>::type dividend,
                const sdivider_t<N2>& divisor) {
  typedef typename divider_uint_t<N1 * 2>::type T1;
  typedef typename divider_uint_t<N1>::type T2;
  T2 s = dividend < 0 ? -1 : 0;
  T1 t = ((T1)(T2)((dividend ^ s) - s)) * divisor.magic1 
        + divisor.magic2;
  T2 u = (T2)(t >> N2) >> divisor.shift;
  s ^= divisor.sign;
  return (u ^ s) - s;
}
template <unsigned N1, unsigned N2>
inline typename divider_int_t<N1>::type
divide_template(const typename divider_int_t<N1>::type dividend,
                const scdivider_t<N2>& divisor) {
  typedef typename divider_uint_t<N1 * 2>::type T1;
  typedef typename divider_uint_t<N1>::type T2;
  
  T2 s = dividend < 0 ? -1 : 0;
  T1 t = ((T1)(T2)((dividend ^ s) - s)) * divisor.magic
        + (divisor.ctrl & 0x80 ? divisor.magic : 0);
  T2 u = (T2)(t >> N2) >> (divisor.ctrl & (N2 - 1));
  s ^= (divisor.ctrl & 0x40 ? -1 : 0);
  return (u ^ s) - s;
}

inline uint16_t divide(const uint16_t dividend, const udivider_t<16>& divisor) {
  return divide_template<16, 16>(dividend, divisor);
}
inline uint32_t divide(const uint32_t dividend, const udivider_t<16>& divisor) {
  return divide_template<32, 16>(dividend, divisor);
}
inline uint32_t divide(const uint32_t dividend, const udivider_t<32>& divisor) {
  return divide_template<32, 32>(dividend, divisor);
}

inline uint16_t divide(const uint16_t dividend, const ucdivider_t<16>& divisor) {
  return divide_template<16, 16>(dividend, divisor);
}
inline uint32_t divide(const uint32_t dividend, const ucdivider_t<16>& divisor) {
  return divide_template<32, 16>(dividend, divisor);
}
inline uint32_t divide(const uint32_t dividend, const ucdivider_t<32>& divisor) {
  return divide_template<32, 32>(dividend, divisor);
}

inline int16_t divide(const int16_t dividend, const sdivider_t<16>& divisor) {
  return divide_template<16, 16>(dividend, divisor);
}
inline int32_t divide(const int32_t dividend, const sdivider_t<16>& divisor) {
  return divide_template<32, 16>(dividend, divisor);
}
inline int32_t divide(const int32_t dividend, const sdivider_t<32>& divisor) {
  return divide_template<32, 32>(dividend, divisor);
}

inline int16_t divide(const int16_t dividend, const scdivider_t<16>& divisor) {
  return divide_template<16, 16>(dividend, divisor);
}
inline int32_t divide(const int32_t dividend, const scdivider_t<16>& divisor) {
  return divide_template<32, 16>(dividend, divisor);
}
inline int32_t divide(const int32_t dividend, const scdivider_t<32>& divisor) {
  return divide_template<32, 32>(dividend, divisor);
}

#endif /* CONST_DIVISION_H */
