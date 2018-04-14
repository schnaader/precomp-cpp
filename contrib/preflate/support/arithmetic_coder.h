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

#ifndef ARITHMETIC_CODER_H
#define ARITHMETIC_CODER_H

#include <stdint.h>
#include <string.h>
#include "bitstream.h"
#include "const_division.h"

class ArithmeticEncoder {
public:
  ArithmeticEncoder(BitOutputStream& bos);
  void flush();
  void encode(const uint32_t scale, const uint32_t low, const uint32_t high) {
    // update steps, low count, high count
    uint32_t step = ((_high - _low) + 1) / scale;
    _high = _low + step * high - 1;
    _low += step * low;
    _normalize();
  }
  void encodeShiftScale(const uint32_t shift, const uint32_t low, const uint32_t high) {
    // update steps, low count, high count
    uint32_t step = ((_high - _low) + 1) >> shift;
    _high = _low + step * high - 1;
    _low += step * low;
    _normalize();
  }
  void encode(const udivider_t<32>& scale, const uint32_t low, const uint32_t high) {
    // update steps, low count, high count
    uint32_t step = divide((_high - _low) + 1, scale);
    _high = _low + step * high - 1;
    _low += step * low;
    _normalize();
  }

private:
  void _normalize();
  void _writeE3(const unsigned w);

  BitOutputStream& _bos;

  // arithmetic coding variables
  uint32_t _low;
  uint32_t _high;
  uint32_t _e3cnt;
};

class ArithmeticDecoder {
public:
  ArithmeticDecoder(BitInputStream& bis);
  unsigned decode(const uint32_t scale, const unsigned bounds[], const unsigned N) {
    uint32_t step = ((_high - _low) + 1) / scale;
    return _decode(step, bounds, N);
  }
  unsigned decodeShiftScale(const uint32_t shift, const unsigned bounds[], const unsigned N) {
    uint32_t step = ((_high - _low) + 1) >> shift;
    return _decode(step, bounds, N);
  }
  unsigned decode(const udivider_t<32>& scale, const unsigned bounds[], const unsigned N) {
    uint32_t step = divide((_high - _low) + 1, scale);
    return _decode(step, bounds, N);
  }

  unsigned decodeBinary(const uint32_t scale, const unsigned bounds[]) {
    uint32_t step = ((_high - _low) + 1) / scale;
    return _decodeBinary(step, bounds);
  }
  unsigned decodeBinaryShiftScale(const uint32_t shift, const unsigned bounds[]) {
    uint32_t step = ((_high - _low) + 1) >> shift;
    return _decodeBinary(step, bounds);
  }
  unsigned decodeBinary(const udivider_t<32>& scale, const unsigned bounds[]) {
    uint32_t step = divide((_high - _low) + 1, scale);
    return _decodeBinary(step, bounds);
  }

private:
  unsigned _findIndex(const unsigned bounds[],
                      const unsigned N,
                      const unsigned val) {
    for (unsigned i = 0; i < N; ++i) {
      if (val < bounds[i + 1]) {
        return i;
      }
    }
    return N - 1;
  }
  unsigned _decode(const uint32_t step, const unsigned bounds[], const unsigned N) {
    uint32_t val = (_value - _low) / step;
    unsigned result = _findIndex(bounds, N, val);
    _high = _low + step * bounds[result + 1] - 1;
    _low += step * bounds[result];
    _normalize();
    return result;
  }
  unsigned _decodeBinary(const uint32_t step, const unsigned bounds[]) {
    unsigned result = (_value >= _low + bounds[1] * step);
    _high = _low + step * bounds[result + 1] - 1;
    _low += step * bounds[result];
    _normalize();
    return result;
  }
  void _normalize();

  BitInputStream& _bis;

  // arithmetic coding variables
  uint32_t _value;
  uint32_t _low;
  uint32_t _high;
};

bool modelCheckFixed(unsigned bounds[], unsigned short ids[], unsigned short rids[],
                     const unsigned N);
void modelSortBounds(unsigned bounds[], unsigned short ids[], unsigned short rids[],
                     unsigned backup[], const unsigned N);
void modelRecreateBounds(unsigned bounds[], const unsigned N);

template <unsigned N>
struct ACModelBase {
  static const unsigned L = N;
  bool isEqualTo(const ACModelBase& m) const {
    for (unsigned i = 0; i < N; ++i) {
      if (bounds[i] != m.bounds[i]) {
        return false;
      }
      if (bounds[i + 1] > 0 && ids[i] != m.ids[i]) {
        return false;
      }
    }
    if (bounds[N] != m.bounds[N]) {
      return false;
    }
    return true;
  }

  unsigned bounds[N + 1];
  unsigned short ids[N], rids[N];
  bool _fixed;
};

struct ACFixedScaleBinaryModel : public ACModelBase<2> {
  ACFixedScaleBinaryModel() {}
  ACFixedScaleBinaryModel(const unsigned(&arr)[2]) {
    memcpy(this->bounds, arr, sizeof(arr));
    build();
  }
  void build();
  void encode(ArithmeticEncoder* encoder, const unsigned item) {
    if (!this->_fixed) {
      unsigned pos = this->rids[item];
      encoder->encodeShiftScale(16, this->bounds[pos], this->bounds[pos + 1]);
    }
  }
#if 0
  unsigned decode(aricoder* codec) {
    symbol s;
    s.scale = 1 << 16;
    unsigned cnt = codec->decode_count(&s);
    for (unsigned i = 0; i < N; ++i) {
      if (cnt < bounds[i + 1]) {
        s.low_count = bounds[i];
        s.high_count = bounds[i + 1];
        codec->decode(&s);
        return ids[i];
      }
    }
    return 0;
  }
#endif
};

template <unsigned N>
struct ACFixedScaleModel : public ACModelBase<N> {
  ACFixedScaleModel() {}
  ACFixedScaleModel(const unsigned(&arr)[N]) {
    memcpy(this->bounds, arr, sizeof(arr));
    build();
  }
  void build() {
    unsigned backup[N];
    if (!(this->_fixed = modelCheckFixed(this->bounds, this->ids, this->rids, N))) {
      modelSortBounds(this->bounds, this->ids, this->rids, backup, N);
      modelRecreateBounds(this->bounds, N);
    }
  }
  void encode(ArithmeticEncoder* encoder, const unsigned item) {
    if (!this->_fixed) {
      unsigned pos =this->rids[item];
      encoder->encodeShiftScale(16, this->bounds[pos], this->bounds[pos + 1]);
    }
  }
#if 0
  unsigned decode(aricoder* codec) {
    symbol s;
    s.scale = 1 << 16;
    unsigned cnt = codec->decode_count(&s);
    for (unsigned i = 0; i < N; ++i) {
      if (cnt < bounds[i + 1]) {
        s.low_count = bounds[i];
        s.high_count = bounds[i + 1];
        codec->decode(&s);
        return this->ids[i];
      }
    }
    return 0;
  }
#endif
};

#endif /* ARITHMETIC_CODER_H */
