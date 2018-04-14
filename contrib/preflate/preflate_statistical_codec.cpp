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
#include "preflate_parameter_estimator.h"
#include "preflate_statistical_codec.h"
#include "preflate_statistical_model.h"
#include "packARI/source/bitops.h"
#include "packARI/source/aricoder.h"
#include "support/array_helper.h"
#include "support/bit_helper.h"
#include <stdint.h>

template <unsigned N>
struct PreflateCodecSubModel {
  static const unsigned L = N;
  PreflateCodecSubModel(const unsigned(&arr)[N]) {
    memcpy(bounds, arr, sizeof(arr));
    build();
  }
  PreflateCodecSubModel() {
  }
  void build() {
    unsigned backup[N];
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
//        bounds[i + 1] = (((uint64_t)acc) << 16) / sum;
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
  void encode(aricoder* codec, const unsigned item) {
    symbol s;
    s.scale = 1 << 16;
    s.low_count = bounds[rids[item]];
    s.high_count = bounds[rids[item] + 1];
    codec->encode(&s);
  }
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
  bool isEqualTo(const PreflateCodecSubModel<N>& m) const {
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
};

template <unsigned N, bool zero>
struct PreflateCodecCorrectionSubModel {
  static const unsigned L = N;
  PreflateCodecCorrectionSubModel(const unsigned(&arr1)[N + zero],
                        const unsigned(&arr2)[N]) 
    : sign()
    , pos() 
    , neg(arr2) {
    memcpy(pos.bounds, arr1 + zero, N * sizeof(unsigned));
    pos.build();

    if (zero) {
      sign.bounds[1] = arr1[0];
    }
    sign.bounds[0] = sumArray(arr1 + zero, N);
    sign.bounds[1 + zero] = sumArray(arr2);
    sign.build();
  }
  PreflateCodecCorrectionSubModel() {
  }
  void encode(aricoder* codec, const int value,
              const unsigned refvalue, 
              const unsigned minvalue,
              const unsigned maxvalue) {
    if (value == 0) {
      sign.encode(codec, 1);
      return;
    }
    if (value > 0) {
      sign.encode(codec, 0);
      if (value >= (int)N) {
        pos.encode(codec, N - 1);
        PreflateStatisticalEncoder::encodeValue(codec, value - N, bitLength(maxvalue - N - refvalue));
      } else {
        pos.encode(codec, value - 1);
      }
    } else {
      sign.encode(codec, 1 + zero);
      if (value <= -(int)N) {
        neg.encode(codec, N - 1);
        PreflateStatisticalEncoder::encodeValue(codec, -value - N, bitLength(refvalue - N - minvalue));
      } else {
        neg.encode(codec, -value - 1);
      }
    }
  }
  int decode(aricoder* codec,
              const unsigned refvalue,
              const unsigned minvalue,
              const unsigned maxvalue) {
    int val = sign.decode(codec);
    if (zero && val == 1) {
      return 0;
    }
    if (val == 0) {
      val = pos.decode(codec);
      if (val >= (int)(N - 1)) {
        return PreflateStatisticalDecoder::decodeValue(codec, bitLength(maxvalue - N - refvalue)) + N;
      } else {
        return val + 1;
      }
    } else {
      val = neg.decode(codec);
      if (val >= (int)(N - 1)) {
        return -PreflateStatisticalDecoder::decodeValue(codec, bitLength(refvalue - N - minvalue)) - N;
      } else {
        return -val - 1;
      }
    }
  }
  bool isEqualTo(const PreflateCodecCorrectionSubModel<N, zero>& m) const {
    return sign.isEqualTo(m.sign)
      && pos.isEqualTo(m.pos)
      && neg.isEqualTo(m.neg);
  }

  PreflateCodecSubModel<2+zero> sign;
  PreflateCodecSubModel<N> pos;
  PreflateCodecSubModel<N> neg;
};

struct PreflateCodecModel {
  // Blocks
  PreflateCodecSubModel<3> blockType;
  PreflateCodecSubModel<2> EOBMisprediction;
  // Tokens
  PreflateCodecSubModel<2> LITMisprediction;
  PreflateCodecSubModel<2> REFMisprediction;
  PreflateCodecSubModel<2> LENMisprediction;
  PreflateCodecSubModel<2> DISTOnlyMisprediction;
  PreflateCodecCorrectionSubModel<6,false> LENCorrection;
  PreflateCodecSubModel<4> DISTAfterLenCorrection;
  PreflateCodecSubModel<4> DISTOnlyCorrection;
  // Trees
  PreflateCodecSubModel<2> TCCountMisprediction;
  PreflateCodecCorrectionSubModel<3,true> TCBitlengthCorrection;
  PreflateCodecSubModel<2> LCountMisprediction;
  PreflateCodecSubModel<2> DCountMisprediction;
  PreflateCodecSubModel<2> LDTypeMisprediction_B;
  PreflateCodecSubModel<2> LDTypeMisprediction_R;
  PreflateCodecSubModel<2> LDTypeMisprediction_0s;
  PreflateCodecSubModel<2> LDTypeMisprediction_0l;
  PreflateCodecSubModel<2>* LDTypeMisprediction[4];
  PreflateCodecSubModel<4> LDTypeReplacement;
  PreflateCodecSubModel<2> LDRepeatCountMisprediction;
  PreflateCodecCorrectionSubModel<4,true> LDBitlengthCorrection;

  PreflateCodecModel(const PreflateStatisticalModel& model)
    : blockType(model.blockType)
    , EOBMisprediction(model.EOBMisprediction)
    , LITMisprediction(model.LITMisprediction)
    , REFMisprediction(model.REFMisprediction)
    , LENMisprediction(model.LENMisprediction)
    , DISTOnlyMisprediction(model.DISTOnlyMisprediction)
    , LENCorrection(model.LENPositiveCorrection, model.LENNegativeCorrection)
    , DISTAfterLenCorrection(model.DISTAfterLenCorrection)
    , DISTOnlyCorrection(model.DISTOnlyCorrection)
    , TCCountMisprediction(model.TCCountMisprediction)
    , TCBitlengthCorrection(model.TCBitlengthPositiveCorrection,
                            model.TCBitlengthNegativeCorrection)
    , LCountMisprediction(model.LCountMisprediction)
    , DCountMisprediction(model.DCountMisprediction)
    , LDTypeMisprediction_B(model.LDTypeMisprediction[0])
    , LDTypeMisprediction_R(model.LDTypeMisprediction[1])
    , LDTypeMisprediction_0s(model.LDTypeMisprediction[2])
    , LDTypeMisprediction_0l(model.LDTypeMisprediction[3])
    , LDTypeMisprediction{&LDTypeMisprediction_B, &LDTypeMisprediction_R,
                             &LDTypeMisprediction_0s, &LDTypeMisprediction_0l}
    , LDTypeReplacement(model.LDTypeReplacement)
    , LDRepeatCountMisprediction(model.LDRepeatCountMisprediction)
    , LDBitlengthCorrection(model.LDBitlengthPositiveCorrection,
                            model.LDBitlengthNegativeCorrection) {
  }
  PreflateCodecModel() 
    : LDTypeMisprediction {&LDTypeMisprediction_B, &LDTypeMisprediction_R,
      &LDTypeMisprediction_0s, &LDTypeMisprediction_0l} {
  }
  bool isEqualTo(const PreflateCodecModel& m) const {
    return blockType.isEqualTo(m.blockType)
      && EOBMisprediction.isEqualTo(m.EOBMisprediction)
      && LITMisprediction.isEqualTo(m.LITMisprediction)
      && REFMisprediction.isEqualTo(m.REFMisprediction)
      && LENMisprediction.isEqualTo(m.LENMisprediction)
      && DISTOnlyMisprediction.isEqualTo(m.DISTOnlyMisprediction)
      && LENCorrection.isEqualTo(m.LENCorrection)
      && DISTAfterLenCorrection.isEqualTo(m.DISTAfterLenCorrection)
      && DISTOnlyCorrection.isEqualTo(m.DISTOnlyCorrection)
      && TCCountMisprediction.isEqualTo(m.TCCountMisprediction)
      && TCBitlengthCorrection.isEqualTo(m.TCBitlengthCorrection)
      && LCountMisprediction.isEqualTo(m.LCountMisprediction)
      && DCountMisprediction.isEqualTo(m.DCountMisprediction)
      && LDTypeMisprediction_B.isEqualTo(m.LDTypeMisprediction_B)
      && LDTypeMisprediction_R.isEqualTo(m.LDTypeMisprediction_R)
      && LDTypeMisprediction_0s.isEqualTo(m.LDTypeMisprediction_0s)
      && LDTypeMisprediction_0l.isEqualTo(m.LDTypeMisprediction_0l)
      && LDTypeReplacement.isEqualTo(m.LDTypeReplacement)
      && LDRepeatCountMisprediction.isEqualTo(m.LDRepeatCountMisprediction)
      && LDBitlengthCorrection.isEqualTo(m.LDBitlengthCorrection);
  }
};

PreflateStatisticalEncoder::PreflateStatisticalEncoder(const PreflateStatisticalModel& model_) {
  data = new iostream(nullptr, TYPE_MEMORY, 0, MODE_WRITE);
  codec = new aricoder(data, MODE_WRITE);
  model = new PreflateCodecModel(model_);
}
PreflateStatisticalEncoder::~PreflateStatisticalEncoder() {
  delete model;
  delete codec;
  delete data;
}

void PreflateStatisticalEncoder::encode(const PreflateCorrectionType& type, const int value, const unsigned refvalue) {
  switch (type) {
    // Block
  case CORR_BLOCK_TYPE:
    model->blockType.encode(codec, value);
    break;
  case CORR_EOB_MISPREDICTION:
    model->EOBMisprediction.encode(codec, value);
    break;
    // Tokens
  case CORR_LIT_MISPREDICTION:
    model->LITMisprediction.encode(codec, value);
    break;
  case CORR_REF_MISPREDICTION:
    model->REFMisprediction.encode(codec, value);
    break;
  case CORR_LEN_MISPREDICTION:
    model->LENMisprediction.encode(codec, value);
    break;
  case CORR_LEN_CORRECTION:
    model->LENCorrection.encode(codec, value, refvalue, 3, 258);
    break;
  case CORR_DIST_AFTER_LEN_CORRECTION:
    if (value >= (int)model->DISTAfterLenCorrection.L - 1) {
      model->DISTAfterLenCorrection.encode(codec, model->DISTAfterLenCorrection.L - 1);
      encodeValue(value - (model->DISTAfterLenCorrection.L - 1), 15);
    } else {
      model->DISTAfterLenCorrection.encode(codec, value);
    }
    break;

  case CORR_DIST_ONLY_MISPREDICTION:
    model->DISTOnlyMisprediction.encode(codec, value);
    break;
  case CORR_DIST_ONLY_CORRECTION:
    if (value >= (int)model->DISTOnlyCorrection.L - 1) {
      model->DISTOnlyCorrection.encode(codec, model->DISTOnlyCorrection.L - 1);
      encodeValue(value - (model->DISTOnlyCorrection.L - 1), 15);
    } else {
      model->DISTOnlyCorrection.encode(codec, value);
    }
    break;
    // Trees
  case CORR_L_COUNT_MISPREDICTION:
    model->LCountMisprediction.encode(codec, value);
    break;
  case CORR_D_COUNT_MISPREDICTION:
    model->DCountMisprediction.encode(codec, value);
    break;
  case CORR_LD_TYPE_MISPREDICTION:
    model->LDTypeMisprediction[refvalue]->encode(codec, value);
    break;
  case CORR_LD_TYPE_REPLACEMENT:
    model->LDTypeReplacement.encode(codec, value);
    break;
  case CORR_LD_REPEAT_MISPREDICTION:
    model->LDRepeatCountMisprediction.encode(codec, value);
    break;
  case CORR_LD_BITLENGTH_CORRECTION:
    model->LDBitlengthCorrection.encode(codec, value, refvalue, 0, 15);
    break;
  case CORR_TC_COUNT_MISPREDICTION:
    model->TCCountMisprediction.encode(codec, value);
    break;
  case CORR_TC_BITLENGTH_CORRECTION:
    model->TCBitlengthCorrection.encode(codec, value, refvalue, 0, 7);
    break;
  }
}
void PreflateStatisticalEncoder::encodeValue(aricoder* codec_, const unsigned value, const unsigned maxBits) {
  symbol s;
  s.scale = 1 << maxBits;
  s.low_count = value;
  s.high_count = value + 1;
  codec_->encode(&s);
}
void PreflateStatisticalEncoder::encodeValue(const unsigned value, const unsigned maxBits) {
  encodeValue(codec, value, maxBits);
}


struct PreflateCodecModelModel {
  PreflateCodecModelModel() {
  }
  template <unsigned N>
  void check(const PreflateCodecSubModel<N>& m) {
    for (unsigned i = 1; i < N; ++i) {
      stat_count++;
      if (!m.bounds[i]) {
        zero_count++;
      }
    }
    end_stat_count++;
    if (!m.bounds[N]) {
      end_zero_count++;
    }
  }
  template <unsigned N, bool Z>
  void check(const PreflateCodecCorrectionSubModel<N,Z>& m) {
    check(m.sign);
    check(m.pos);
    check(m.neg);
  }
  void check(const PreflateCodecModel& m) {
    zero_count = stat_count = 0;
    end_zero_count = end_stat_count = 0;
    // Blocks
    check(m.blockType);
    check(m.EOBMisprediction);
    // Tokens
    check(m.LITMisprediction);
    check(m.REFMisprediction);
    check(m.LENMisprediction);
    check(m.DISTOnlyMisprediction);
    check(m.LENCorrection);
    check(m.DISTAfterLenCorrection);
    check(m.DISTOnlyCorrection);
    // Trees
    check(m.TCCountMisprediction);
    check(m.TCBitlengthCorrection);
    check(m.LCountMisprediction);
    check(m.DCountMisprediction);
    check(m.LDTypeMisprediction_B);
    check(m.LDTypeMisprediction_R);
    check(m.LDTypeMisprediction_0s);
    check(m.LDTypeMisprediction_0l);
    check(m.LDTypeReplacement);
    check(m.LDRepeatCountMisprediction);
    check(m.LDBitlengthCorrection);
  }

  void encodeVal(aricoder* codec, const unsigned val) {
    unsigned bits = bitLength(val);
    // encode shift
    PreflateStatisticalEncoder::encodeValue(codec, bits - 1, 4);
    // and precision
    if (bits >= 5) {
      PreflateStatisticalEncoder::encodeValue(codec, (val >> (bits - 5)) & 0xf, 4);
    } else {
      PreflateStatisticalEncoder::encodeValue(codec, val & ~(1 << (bits - 1)), bits - 1);
    }
  }
  void encodeId(aricoder* codec, 
                const unsigned id, const unsigned count) {
    unsigned bits = bitLength(count - 1);
    PreflateStatisticalEncoder::encodeValue(codec, id, bits);
  }

  template <unsigned N>
  void encode(aricoder* codec, const PreflateCodecSubModel<N>& m) {
    // Mark zero values
    for (unsigned i = 1; i < N + 1; ++i) {
      if (!m.bounds[i]) {
        codec->encode(i == N ? &ez : &z);
      } else {
        codec->encode(i == N ? &enz : &nz);
        break;
      }
    }
    // Transmit values
    for (unsigned i = 1; i < N; ++i) {
      if (m.bounds[i]) {
        encodeVal(codec, m.bounds[i] - m.bounds[i - 1]);
      }
    }
    // Transmit ids
    for (unsigned i = 0; i < N; ++i) {
      if (m.bounds[i + 1]) {
        encodeId(codec, m.ids[i], N);
      }
    }
  }
  template <unsigned N, bool Z>
  void encode(aricoder* codec, const PreflateCodecCorrectionSubModel<N, Z>& m) {
    encode(codec, m.sign);
    encode(codec, m.pos);
    encode(codec, m.neg);
  }
  void encode(aricoder* codec, const PreflateCodecModel& m) {
    PreflateStatisticalEncoder::encodeValue(codec, zero_count, bitLength(stat_count));
    PreflateStatisticalEncoder::encodeValue(codec, end_zero_count, bitLength(end_stat_count));

    nz.scale = z.scale = stat_count; // current number of statistics
    z.low_count = 0;
    nz.low_count = z.high_count = zero_count;
    nz.high_count = nz.scale;

    enz.scale = ez.scale = end_stat_count; // current number of statistics
    ez.low_count = 0;
    enz.low_count = ez.high_count = end_zero_count;
    enz.high_count = end_stat_count;

    // Blocks
    encode(codec, m.blockType);
    encode(codec, m.EOBMisprediction);
    // Tokens
    encode(codec, m.LITMisprediction);
    encode(codec, m.REFMisprediction);
    encode(codec, m.LENMisprediction);
    encode(codec, m.DISTOnlyMisprediction);
    encode(codec, m.LENCorrection);
    encode(codec, m.DISTAfterLenCorrection);
    encode(codec, m.DISTOnlyCorrection);
    // Trees
    encode(codec, m.TCCountMisprediction);
    encode(codec, m.TCBitlengthCorrection);
    encode(codec, m.LCountMisprediction);
    encode(codec, m.DCountMisprediction);
    encode(codec, m.LDTypeMisprediction_B);
    encode(codec, m.LDTypeMisprediction_R);
    encode(codec, m.LDTypeMisprediction_0s);
    encode(codec, m.LDTypeMisprediction_0l);
    encode(codec, m.LDTypeReplacement);
    encode(codec, m.LDRepeatCountMisprediction);
    encode(codec, m.LDBitlengthCorrection);
  }

  unsigned decodeVal(aricoder* codec) {
    // encode shift
    unsigned bits = PreflateStatisticalDecoder::decodeValue(codec, 4) + 1;
    // and precision
    if (bits >= 5) {
      return (PreflateStatisticalDecoder::decodeValue(codec, 4) | 0x10) << (bits - 5);
    } else {
      return PreflateStatisticalDecoder::decodeValue(codec, bits - 1) | (1 << (bits - 1));
    }
  }
  unsigned decodeId(aricoder* codec, const unsigned count) {
    unsigned bits = bitLength(count - 1);
    return PreflateStatisticalDecoder::decodeValue(codec, bits);
  }

  template <unsigned N>
  void decode(aricoder* codec, PreflateCodecSubModel<N>& m) {
    // Mark zero values
    unsigned i;
    m.bounds[0] = 0;
    for (i = 1; i < N + 1; ++i) {
      symbol* s = i == N ? &enz : &nz;
      unsigned cnt = codec->decode_count(s);
      if (cnt >= s->low_count) {
        codec->decode(s);
        break;
      }
      codec->decode(i == N ? &ez : &z);
      m.bounds[i] = 0;
    }
    if (i < N + 1) {
      m.bounds[N] = 1 << 16;
    }
    // Transmit values
    unsigned j = i;
    for (; i < N; ++i) {
      m.bounds[i] = m.bounds[i - 1] + decodeVal(codec);
    }
    // Transmit ids
    for (i = j - 1; i < N; ++i) {
      m.ids[i] = decodeId(codec, N);
    }
  }
  template <unsigned N, bool Z>
  void decode(aricoder* codec, PreflateCodecCorrectionSubModel<N, Z>& m) {
    decode(codec, m.sign);
    decode(codec, m.pos);
    decode(codec, m.neg);
  }
  void decode(aricoder* codec, PreflateCodecModel& m) {
    zero_count = PreflateStatisticalDecoder::decodeValue(codec, bitLength(stat_count));
    end_zero_count = PreflateStatisticalDecoder::decodeValue(codec, bitLength(end_stat_count));

    nz.scale = z.scale = stat_count; // current number of statistics
    z.low_count = 0;
    nz.low_count = z.high_count = zero_count;
    nz.high_count = nz.scale;

    enz.scale = ez.scale = end_stat_count; // current number of statistics
    ez.low_count = 0;
    enz.low_count = ez.high_count = end_zero_count;
    enz.high_count = end_stat_count;

    // Blocks
    decode(codec, m.blockType);
    decode(codec, m.EOBMisprediction);
    // Tokens
    decode(codec, m.LITMisprediction);
    decode(codec, m.REFMisprediction);
    decode(codec, m.LENMisprediction);
    decode(codec, m.DISTOnlyMisprediction);
    decode(codec, m.LENCorrection);
    decode(codec, m.DISTAfterLenCorrection);
    decode(codec, m.DISTOnlyCorrection);
    // Trees
    decode(codec, m.TCCountMisprediction);
    decode(codec, m.TCBitlengthCorrection);
    decode(codec, m.LCountMisprediction);
    decode(codec, m.DCountMisprediction);
    decode(codec, m.LDTypeMisprediction_B);
    decode(codec, m.LDTypeMisprediction_R);
    decode(codec, m.LDTypeMisprediction_0s);
    decode(codec, m.LDTypeMisprediction_0l);
    decode(codec, m.LDTypeReplacement);
    decode(codec, m.LDRepeatCountMisprediction);
    decode(codec, m.LDBitlengthCorrection);
  }

  unsigned stat_count, zero_count;
  unsigned end_stat_count, end_zero_count;
  symbol z, nz, ez, enz;
};

void PreflateStatisticalEncoder::encodeModel() {
  PreflateCodecModelModel modelmodel;
  modelmodel.check(*model);
  modelmodel.encode(codec, *model);
}

void PreflateStatisticalEncoder::encodeHeader() {
  encodeValue('P', 8);
  encodeValue('F', 8);
  encodeValue(0x01, 8);
}

void PreflateStatisticalEncoder::encodeParameters(const PreflateParameters& params) {
  encodeValue(params.zlibCompatible, 1);
  encodeValue(params.windowBits - 8, 3);
  encodeValue(params.memLevel, 4);
  encodeValue(params.compLevel, 4);
  encodeValue(params.strategy, 2);
  encodeValue(params.huffStrategy, 2);
  encodeValue(params.farLen3MatchesDetected, 1);
  encodeValue(params.veryFarMatchesDetected, 1);
  encodeValue(params.matchesToStartDetected, 1);
  encodeValue(params.log2OfMaxChainDepthM1, 4);
}
std::vector<unsigned char>  PreflateStatisticalEncoder::encodeFinish() {
  delete codec;
  codec = nullptr;
  std::vector<unsigned char> result(data->getptr(), data->getptr() + data->getsize());
  delete data;
  data = nullptr;
//  delete model;
//  model = nullptr;
  return result;
}


PreflateStatisticalDecoder::PreflateStatisticalDecoder(const std::vector<unsigned char>& x) : storage(x) {
  data = new iostream(&storage[0], TYPE_MEMORY, storage.size(), MODE_READ);
  codec = new aricoder(data, MODE_READ);
  model = nullptr;
}
PreflateStatisticalDecoder::~PreflateStatisticalDecoder() {
  delete model;
  delete codec;
  delete data;
}

bool PreflateStatisticalDecoder::decodeHeader() {
  if (decodeValue(8) != 'P') {
    return false;
  }
  if (decodeValue(8) != 'F') {
    return false;
  }
  if (decodeValue(8) != 0x01) {
    return false;
  }
  return true;
}
bool PreflateStatisticalDecoder::decodeParameters(PreflateParameters& params) {
  params.zlibCompatible = decodeValue(1) != 0;
  params.windowBits = decodeValue(3) + 8;
  params.memLevel = decodeValue(4);
  params.compLevel = decodeValue(4);

  unsigned n = decodeValue(2);
  switch (n) {
  case PREFLATE_DEFAULT:
    params.strategy = PREFLATE_DEFAULT;
    break;
  case PREFLATE_RLE_ONLY:
    params.strategy = PREFLATE_RLE_ONLY;
    break;
  case PREFLATE_HUFF_ONLY:
    params.strategy = PREFLATE_HUFF_ONLY;
    break;
  case PREFLATE_STORE:
    params.strategy = PREFLATE_STORE;
    break;
  }
  n = decodeValue(2);
  switch (n) {
  case PREFLATE_HUFF_DYNAMIC:
    params.huffStrategy = PREFLATE_HUFF_DYNAMIC;
    break;
  case PREFLATE_HUFF_MIXED:
    params.huffStrategy = PREFLATE_HUFF_MIXED;
    break;
  case PREFLATE_HUFF_STATIC:
    params.huffStrategy = PREFLATE_HUFF_STATIC;
    break;
  }
  params.farLen3MatchesDetected = decodeValue(1) != 0;
  params.veryFarMatchesDetected = decodeValue(1) != 0;
  params.matchesToStartDetected = decodeValue(1) != 0;
  params.log2OfMaxChainDepthM1 = decodeValue(4);
  return params.compLevel >= 1 && params.compLevel <= 9
    && params.memLevel >= 1 && params.memLevel <= 9
    && params.windowBits >= 8 && params.windowBits <= 15;
}
void PreflateStatisticalDecoder::decodeFinish() {
  delete codec;
  codec = nullptr;
  delete data;
  data = nullptr;
  delete model;
  model = nullptr;
  storage.clear();
}

int PreflateStatisticalDecoder::decode(const PreflateCorrectionType& type, const unsigned refvalue) {
  switch (type) {
    // Block
  case CORR_BLOCK_TYPE:
    return model->blockType.decode(codec);
  case CORR_EOB_MISPREDICTION:
    return model->EOBMisprediction.decode(codec);
    // Tokens
  case CORR_LIT_MISPREDICTION:
    return model->LITMisprediction.decode(codec);
  case CORR_REF_MISPREDICTION:
    return model->REFMisprediction.decode(codec);
  case CORR_LEN_MISPREDICTION:
    return model->LENMisprediction.decode(codec);
  case CORR_LEN_CORRECTION:
    return model->LENCorrection.decode(codec, refvalue, 3, 258);
  case CORR_DIST_AFTER_LEN_CORRECTION: {
    unsigned val = model->DISTAfterLenCorrection.decode(codec);
    if (val >= model->DISTAfterLenCorrection.L - 1) {
      return decodeValue(15) + (model->DISTAfterLenCorrection.L - 1);
    } else {
      return val;
    }
  }
  case CORR_DIST_ONLY_MISPREDICTION:
    return model->DISTOnlyMisprediction.decode(codec);
  case CORR_DIST_ONLY_CORRECTION: {
    unsigned val = model->DISTOnlyCorrection.decode(codec);
    if (val >= model->DISTOnlyCorrection.L - 1) {
      return decodeValue(15) + (model->DISTOnlyCorrection.L - 1);
    } else {
      return val;
    }
  }
    // Trees
  case CORR_L_COUNT_MISPREDICTION:
    return model->LCountMisprediction.decode(codec);
  case CORR_D_COUNT_MISPREDICTION:
    return model->DCountMisprediction.decode(codec);
  case CORR_LD_TYPE_MISPREDICTION:
    return model->LDTypeMisprediction[refvalue]->decode(codec);
  case CORR_LD_TYPE_REPLACEMENT:
    return model->LDTypeReplacement.decode(codec);
  case CORR_LD_REPEAT_MISPREDICTION:
    return model->LDRepeatCountMisprediction.decode(codec);
  case CORR_LD_BITLENGTH_CORRECTION:
    return model->LDBitlengthCorrection.decode(codec, refvalue, 0, 15);
  case CORR_TC_COUNT_MISPREDICTION:
    return model->TCCountMisprediction.decode(codec);
  case CORR_TC_BITLENGTH_CORRECTION:
    return model->TCBitlengthCorrection.decode(codec, refvalue, 0, 7);
  default:
    return 0;
  }
}
unsigned PreflateStatisticalDecoder::decodeValue(aricoder* codec_, const unsigned maxBits) {
  symbol s;
  s.scale = 1 << maxBits;
  unsigned cnt = codec_->decode_count(&s);
  s.low_count = cnt;
  s.high_count = cnt + 1;
  codec_->decode(&s);
  return cnt;
}
unsigned PreflateStatisticalDecoder::decodeValue(const unsigned maxBits) {
  return decodeValue(codec, maxBits);
}

bool PreflateStatisticalDecoder::decodeModel() {
  delete model;
  model = new PreflateCodecModel;
  PreflateCodecModelModel modelmodel;
  modelmodel.check(*model);
  modelmodel.decode(codec, *model);
  return true;
}

bool isEqual(const PreflateCodecModel& m1, const PreflateCodecModel& m2) {
  return m1.isEqualTo(m2);
}
