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
#include "arithmetic_coder.h"
#include "array_helper.h"
#include "bit_helper.h"

const uint8_t ArithmeticCodecBase::_normCheckLUT[8] = {
  0x33, 0x77, 0xff, 0xff, 0x33, 0x77, 0xff, 0xff
};

ArithmeticCodecBase::ArithmeticCodecBase()
  : _low(0)
  , _high(0x7fffffff) {}


ArithmeticEncoder::ArithmeticEncoder(BitOutputStream& bos)
  : _bos(bos)
  , _e3cnt(0) {}

void ArithmeticEncoder::_writeE3(const unsigned w) {
  while (_e3cnt > 0) {
    uint32_t todo = std::min(_e3cnt, 16u);
    _bos.put(w, todo);
    _e3cnt -= todo;
  }
}

void ArithmeticEncoder::flush() {
  if (_low < 0x20000000) { // case a.) 
    _bos.put(2, 2); // write 0, 1, E3
    _writeE3(~0u);
  } else {
    _bos.put(1, 1);
  }
  _low = 0;
  _high = 0x7fffffff;
}

void ArithmeticEncoder::_normalize() {
#ifdef _DEBUG
  _ASSERT(_low <= _high && _high < 0x80000000);
#endif
  // write determinated bits
  // this is the case if _low features 1 bits
  // or _high features 0 bits
  uint32_t lh = ~_low & _high;
  if ((lh & 0x40000000) == 0) {
    unsigned w = (_low & 0x40000000) != 0;
    _bos.put(w, 1);
    _writeE3(w - 1);
    if ((lh & 0x20000000) == 0) {
      unsigned l = bitLeadingZeroes((lh << 2) + 3);
      if (l <= 16) {
        _bos.putReverse(_low >> (30 - l), l);
      } else {
        _bos.putReverse(_low >> (30 - 16), 16);
        _bos.putReverse(_low >> (30 - l), l - 16);
      }
      _low  = (_low << (l + 1)) & 0x7fffffff;
      _high = (((_high + 1) << (l + 1)) - 1) & 0x7fffffff;
    } else {
      _low = (_low << 1) & 0x7fffffff;
      _high = ((_high << 1) + 1) & 0x7fffffff;
    }
  }

  // count indeterminated bits
  lh = ~_low | _high;
  if ((lh & 0x20000000) == 0) {
    // low starts with 01, high starts with 10
    unsigned l = bitLeadingZeroes((lh << 2) + 3);
    _e3cnt += l;
    _low = (_low << l) & 0x3fffffff;
    _high = ((((_high + 1) << l) - 1) & 0x3fffffff)
            | 0x40000000;
  }
#ifdef _DEBUG
  _ASSERT(_low <= _high && _high < 0x80000000);
#endif
}

ArithmeticDecoder::ArithmeticDecoder(BitInputStream& bis) 
  : _bis(bis)
  , _value(0) {
  _value = _bis.getReverse(16) << 15;
  _value |= _bis.getReverse(15);
}
void ArithmeticDecoder::_normalize() {
#ifdef _DEBUG
  _ASSERT(_low <= _value && _value <= _high && _high < 0x80000000);
#endif
  // skip determinated bits
  // this is the case if _low features 1 bits
  // or _high features 0 bits
  uint32_t lh = ~_low & _high;
  if ((lh & 0x40000000) == 0) {
    //unsigned w = (_low & 0x40000000) != 0;
    if ((lh & 0x20000000) == 0) {
      unsigned l = bitLeadingZeroes((lh << 2) + 3);
      _low = (_low << (l + 1)) & 0x7fffffff;
      _high = (((_high + 1) << (l + 1)) - 1) & 0x7fffffff;
      if (l <= 15) {
        _value = ((_value << (l + 1)) + _bis.getReverse(l + 1)) & 0x7fffffff;
      } else {
        _value = ((_value << 16) + _bis.getReverse(16)) & 0x7fffffff;
        _value = ((_value << (l - 15)) + _bis.getReverse(l - 15)) & 0x7fffffff;
      }
    } else {
      _low = (_low << 1) & 0x7fffffff;
      _high = ((_high << 1) + 1) & 0x7fffffff;
      _value = ((_value << 1) + _bis.get(1)) & 0x7fffffff;
    }
  }

  // count indeterminated bits
  lh = ~_low | _high;
  if ((lh & 0x20000000) == 0) {
    // low starts with 01, high starts with 10
    unsigned l = bitLeadingZeroes((lh << 2) + 3);
    _low = (_low << l) & 0x3fffffff;
    _high = ((((_high + 1) << l) - 1) & 0x3fffffff)
      | 0x40000000;
    if (l <= 16) {
      _value = (((_value << l) + _bis.getReverse(l)) -0x40000000) & 0x7fffffff;
    } else {
      _value = ((_value << 16) + _bis.getReverse(16));
      _value = (((_value << (l - 16)) + _bis.getReverse(l - 16)) - 0x40000000) & 0x7fffffff;
    }
  }
#ifdef _DEBUG
  _ASSERT(_low <= _value && _value <= _high && _high < 0x80000000);
#endif
}

bool modelCheckFixed(unsigned bounds[], unsigned short ids[], unsigned short rids[],
                     const unsigned N) {
  unsigned idx = N;
  for (unsigned i = 0; i < N; ++i) {
    if (bounds[i]) {
      if (idx != N) {
        return false;
      }
      idx = i;
    }
  }
  ids[N - 1] = idx;
  rids[idx] = N - 1;
  bounds[idx] = 0;
  bounds[N] = 1 << 16;
  return true;
}

void modelSortBounds(unsigned bounds[], unsigned short ids[], unsigned short rids[],
                     unsigned backup[], const unsigned N) {
  for (unsigned i = 0; i < N; ++i) {
    ids[i] = i;
    backup[i] = bounds[i];
  }
  std::sort(ids, ids + N, [=](unsigned i1, unsigned i2) {
    if (backup[i1] != backup[i2]) {
      return backup[i1] < backup[i2];
    }
    return i1 < i2;
  });
  for (unsigned i = 0; i < N; ++i) {
    bounds[i] = backup[ids[i]];
    rids[ids[i]] = i;
  }
}

void modelRecreateBounds(unsigned bounds[], const unsigned N) {
  unsigned sum = sumArray(bounds, N), acc, prev;
  prev = bounds[0];
  bounds[0] = acc = 0;
  for (unsigned i = 0; i < N; ++i) {
    if (prev) {
      acc += prev;
      prev = bounds[i + 1];
      int diff = (((uint64_t)acc) << 16) / sum - bounds[i];
      unsigned diff_bits = bitLength(diff);
      const unsigned k = 5;
      if (diff > 0 && diff_bits > k) {
        diff = diff & (((1 << k) - 1) << (diff_bits - k));
      }
      bounds[i + 1] = bounds[i] + diff;
      if (bounds[i + 1] <= bounds[i]) {
        bounds[i + 1] = bounds[i] + 1;
      }
    } else {
      prev = bounds[i + 1];
      bounds[i + 1] = bounds[i];
    }
  }
  if (bounds[N] > 0) {
    bounds[N] = 1 << 16;
  }
}

void ACFixedScaleBinaryModel::build() {
  if (bounds[0] == 0 || bounds[1] == 0) {
    _fixed = true;
    ids[1] = bounds[0] == 0;
    rids[ids[1]] = 1;
    bounds[1] = bounds[0] = 0;
    bounds[2] = 1 << 16;
    return;
  }
  ids[0] = 0;
  ids[1] = 1;
  if (bounds[1] < bounds[0]) {
    std::swap(ids[0], ids[1]);
    std::swap(bounds[0], bounds[1]);
  }
  rids[ids[0]] = 0;
  rids[ids[1]] = 1;

  modelRecreateBounds(bounds, 2);
}
