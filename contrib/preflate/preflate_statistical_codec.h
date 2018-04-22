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

#ifndef PREFLATE_STATISTICAL_CODEC_H
#define PREFLATE_STATISTICAL_CODEC_H

#include <vector>
#include "packARI/source/bitops.h"
#include "packARI/source/aricoder.h"
#include "support/bit_helper.h"
#include "support/bitstream.h"
#include "support/memstream.h"
#include "preflate_parameter_estimator.h"
#include "preflate_statistical_model.h"

template <unsigned N>
struct PreflateSubModel {
  static const unsigned L = N;
  PreflateSubModel() {}

  void build(const unsigned(&arr)[N], const unsigned defval, const uint8_t prec = 16) {
    build_impl(arr, defval, prec);
  }
  void buildDefault(const unsigned defval);
  void extract(unsigned(&arr)[N]) {
    for (unsigned i = 0; i < N; ++i) {
      arr[i] = bounds[rids[i] + 1] - bounds[rids[i]];
    }
  }
  void read(aricoder*, const uint8_t);
  void write(aricoder*, const uint8_t) const;
  void encode(aricoder* codec, const unsigned item) const {
    symbol s;
    s.scale = 1 << 16;
    s.low_count = bounds[rids[item]];
    s.high_count = bounds[rids[item] + 1];
    codec->encode(&s);
  }
  unsigned decode(aricoder* codec) const {
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
  bool isEqualTo(const PreflateSubModel<N>& m) const;

  unsigned bounds[N + 1];
  unsigned short ids[N + 1], rids[N + 1];
  bool isDefault;

private:
  void build_impl(const unsigned* arr, const unsigned defval, const uint8_t prec);
  template <unsigned NEG, unsigned POS>
  friend struct PreflateCorrectionSubModel;
};

template <unsigned NEG, unsigned POS>
struct PreflateCorrectionSubModel {
  static const unsigned LNEG = NEG;
  static const unsigned LPOS = POS;
  PreflateCorrectionSubModel() {}
  void build(const unsigned(&arr)[NEG + 1 + POS], const int defval, const uint8_t prec = 16) {
    build_impl(arr, defval, prec);
  }
  void buildDefault(const unsigned defval);
  void read(aricoder*, const uint8_t);
  void write(aricoder*, const uint8_t) const;
  void encode(aricoder* codec, const unsigned actvalue,
              const unsigned refvalue,
              const unsigned minvalue,
              const unsigned maxvalue) {
    int diff = actvalue - refvalue;
    if (diff == 0) {
      sign.encode(codec, 0);
      return;
    }
    if (diff > 0) {
      sign.encode(codec, 1);
      if (diff >= (int)POS) {
        pos.encode(codec, POS - 1);
        PreflateBaseModel::encodeValue(codec, diff - POS, bitLength(maxvalue - POS - refvalue));
      } else {
        pos.encode(codec, diff - 1);
      }
    } else {
      sign.encode(codec, 2);
      if (-diff >= (int)NEG) {
        neg.encode(codec, NEG - 1);
        PreflateBaseModel::encodeValue(codec, -diff - NEG, bitLength(refvalue - NEG - minvalue));
      } else {
        neg.encode(codec, -diff - 1);
      }
    }
  }
  unsigned decode(aricoder* codec,
             const unsigned refvalue,
             const unsigned minvalue,
             const unsigned maxvalue) {
    unsigned s = sign.decode(codec);
    if (s == 0) {
      return refvalue;
    }
    if (s == 1) {
      int diff = pos.decode(codec);
      if (diff >= (int)(POS - 1)) {
        return refvalue + PreflateBaseModel::decodeValue(codec, bitLength(maxvalue - POS - refvalue)) + POS;
      } else {
        return refvalue + diff + 1;
      }
    } else {
      int diff = neg.decode(codec);
      if (diff >= (int)(NEG - 1)) {
        return refvalue - PreflateBaseModel::decodeValue(codec, bitLength(refvalue - NEG - minvalue)) - NEG;
      } else {
        return refvalue - diff - 1;
      }
    }
  }
  bool isEqualTo(const PreflateCorrectionSubModel<NEG, POS>& m) const;

  PreflateSubModel<3> sign;
  PreflateSubModel<POS> pos;
  PreflateSubModel<NEG> neg;
  bool isDefault;

private:
  void build_impl(const unsigned* arr, const int defval, const uint8_t prec);
};

struct PreflateModelCodec {
  PreflateSubModel<2> nonDefaultValue;
  uint8_t MBprecision;
  uint8_t MBprecisionP1;
  bool blockFullDefault;
  bool treecodeFullDefault;
  bool tokenFullDefault;
  unsigned totalModels, defaultingModels;

  PreflateModelCodec();
  void initDefault();
  void read(const PreflateStatisticsCounter&);
  void readFromStream(aricoder*);
  void writeToStream(aricoder*);
};

struct PreflateBaseModel {
public:
  PreflateBaseModel();
  void setStream(aricoder*);

  static void encodeValue(aricoder* codec, const unsigned value, const unsigned maxBits) {
    symbol s;
    s.scale = 1 << maxBits;
    s.low_count = value;
    s.high_count = value + 1;
    codec->encode(&s);
  }
  void encodeValue(const unsigned value, const unsigned maxBits) {
    encodeValue(codec, value, maxBits);
  }
  static unsigned decodeValue(aricoder* codec, const unsigned maxBits) {
    symbol s;
    s.scale = 1 << maxBits;
    unsigned cnt = codec->decode_count(&s);
    s.low_count = cnt;
    s.high_count = cnt + 1;
    codec->decode(&s);
    return cnt;
  }
  unsigned decodeValue(const unsigned maxBits) {
    return decodeValue(codec, maxBits);
  }

protected:
  template <unsigned N>
  void readSubModel(PreflateSubModel<N>& sm, const bool isFullDef, const PreflateModelCodec& cc,
                    const unsigned defVal, const uint8_t prec = 16);

  template <unsigned N, unsigned M>
  void readSubModel(PreflateCorrectionSubModel<N, M>& sm, const bool isFullDef, const PreflateModelCodec& cc,
                    const unsigned defVal, const uint8_t prec = 16);

  template <unsigned N>
  void writeSubModel(const PreflateSubModel<N>& sm, const bool isFullDef, const PreflateModelCodec& cc,
                     const unsigned defVal, const uint8_t prec = 16);

  template <unsigned N, unsigned M>
  void writeSubModel(const PreflateCorrectionSubModel<N, M>& sm, const bool isFullDef, const PreflateModelCodec& cc,
                     const unsigned defVal, const uint8_t prec = 16);

  aricoder* codec;
};

struct PreflateBlockPredictionModel : public PreflateBaseModel {
public:
  void read(const PreflateStatisticsCounter::BlockPrediction&, const PreflateModelCodec&);
  void readFromStream(const PreflateModelCodec&);
  void writeToStream(const PreflateModelCodec&);

  unsigned decodeBlockType() {
    return blockType.decode(codec);
  }
  bool decodeEOBMisprediction() {
    return EOBMisprediction.decode(codec);
  }
  bool decodeNonZeroPadding() {
    return nonZeroPadding.decode(codec);
  }

  void encodeBlockType(const unsigned type) {
    blockType.encode(codec, type);
  }
  void encodeEOBMisprediction(const bool misprediction) {
    EOBMisprediction.encode(codec, misprediction);
  }
  void encodeNonZeroPadding(const bool nonzeropadding) {
    nonZeroPadding.encode(codec, nonzeropadding);
  }

  bool isEqualTo(const PreflateBlockPredictionModel& m) const;

private:
  PreflateSubModel<3> blockType;
  PreflateSubModel<2> EOBMisprediction;
  PreflateSubModel<2> nonZeroPadding;
  unsigned precision;
};

struct PreflateTreeCodePredictionModel : public PreflateBaseModel  {
public:
  void read(const PreflateStatisticsCounter::TreeCodePrediction&, const PreflateModelCodec& cc);
  void readFromStream(const PreflateModelCodec& cc);
  void writeToStream(const PreflateModelCodec& cc);

  bool decodeTreeCodeCountMisprediction() {
    return TCCountMisprediction.decode(codec);
  }
  bool decodeLiteralCountMisprediction() {
    return LCountMisprediction.decode(codec);
  }
  bool decodeDistanceCountMisprediction() {
    return DCountMisprediction.decode(codec);
  }
  int decodeTreeCodeBitLengthCorrection(unsigned predval) {
    return TCBitlengthCorrection.decode(codec, predval, 0, 7);
  }
  unsigned decodeLDTypeCorrection(unsigned predtype) {
    return DerivedLDTypeReplacement[predtype].decode(codec);
  }
  unsigned decodeRepeatCountCorrection(const unsigned predval, const unsigned ldtype) {
    static const uint8_t minVal[4] = {0, 3, 3, 11};
    static const uint8_t lenVal[4] = {0, 3, 7, 127};
    return LDRepeatCountCorrection.decode(codec, predval, minVal[ldtype], minVal[ldtype] + lenVal[ldtype]);
  }
  int decodeLDBitLengthCorrection(unsigned predval) {
    return LDBitlengthCorrection.decode(codec, predval, 0, 15);
  }

  void encodeTreeCodeCountMisprediction(const bool misprediction) {
    TCCountMisprediction.encode(codec, misprediction);
  }
  void encodeLiteralCountMisprediction(const bool misprediction) {
    LCountMisprediction.encode(codec, misprediction);
  }
  void encodeDistanceCountMisprediction(const bool misprediction) {
    DCountMisprediction.encode(codec, misprediction);
  }
  void encodeTreeCodeBitLengthCorrection(const unsigned predval, const unsigned actval) {
    TCBitlengthCorrection.encode(codec, actval, predval, 0, 7);
  }
  void encodeLDTypeCorrection(const unsigned predval, const unsigned actval) {
    DerivedLDTypeReplacement[predval].encode(codec, actval);
  }
  void encodeRepeatCountCorrection(const unsigned predval, const unsigned actval, unsigned ldtype) {
    static const uint8_t minVal[4] = {0, 3, 3, 11};
    static const uint8_t lenVal[4] = {0, 3, 7, 127};
    LDRepeatCountCorrection.encode(codec, actval, predval, minVal[ldtype], minVal[ldtype] + lenVal[ldtype]);
  }
  void encodeLDBitLengthCorrection(const unsigned predval, const unsigned actval) {
    LDBitlengthCorrection.encode(codec, actval, predval, 0, 15);
  }

  bool isEqualTo(const PreflateTreeCodePredictionModel& m) const;

private:
  void deriveLDTypeReplacement();

  PreflateSubModel<2> TCCountMisprediction;
  PreflateSubModel<2> LCountMisprediction;
  PreflateSubModel<2> DCountMisprediction;
  PreflateSubModel<2> LDTypeMisprediction[4];
  PreflateSubModel<4> LDTypeReplacementBase;
  PreflateCorrectionSubModel<1, 1> LDRepeatCountCorrection;
  PreflateCorrectionSubModel<3, 3> TCBitlengthCorrection;
  PreflateCorrectionSubModel<4, 4> LDBitlengthCorrection;
  PreflateSubModel<4> DerivedLDTypeReplacement[4];
};

struct PreflateTokenPredictionModel : public PreflateBaseModel {
public:
  void read(const PreflateStatisticsCounter::TokenPrediction&, const PreflateModelCodec& cc);
  void readFromStream(const PreflateModelCodec& cc);
  void writeToStream(const PreflateModelCodec& cc);

  bool decodeLiteralPredictionWrong() {
    return LITMisprediction.decode(codec);
  }
  bool decodeReferencePredictionWrong() {
    return REFMisprediction.decode(codec);
  }
  int decodeLenCorrection(const unsigned predval) {
    return LENCorrection.decode(codec, predval, 3, 258);
  }
  unsigned decodeDistOnlyCorrection() {
    return DISTOnlyCorrection.decode(codec, 0, 0, 32767);
  }
  unsigned decodeDistAfterLenCorrection() {
    return DISTAfterLenCorrection.decode(codec, 0, 0, 32767);
  }
  bool decodeIrregularLen258() {
    return IrregularLen258Encoding.decode(codec);
  }

  void encodeLiteralPredictionWrong(const bool misprediction) {
    LITMisprediction.encode(codec, misprediction);
  }
  void encodeReferencePredictionWrong(const bool misprediction) {
    REFMisprediction.encode(codec, misprediction);
  }
  void encodeLenCorrection(const unsigned predval, const unsigned actval) {
    LENCorrection.encode(codec, actval, predval, 3, 258);
  }
  void encodeDistOnlyCorrection(const unsigned hops) {
    DISTOnlyCorrection.encode(codec, hops, 0, 0, 32767);
  }
  void encodeDistAfterLenCorrection(const unsigned hops) {
    DISTAfterLenCorrection.encode(codec, hops, 0, 0, 32767);
  }
  void encodeIrregularLen258(const bool irregular) {
    IrregularLen258Encoding.encode(codec, irregular);
  }

  bool isEqualTo(const PreflateTokenPredictionModel& m) const;

private:
  PreflateSubModel<2> LITMisprediction;
  PreflateSubModel<2> REFMisprediction;
  PreflateCorrectionSubModel<6, 6> LENCorrection;
  PreflateCorrectionSubModel<0, 3> DISTAfterLenCorrection;
  PreflateCorrectionSubModel<0, 3> DISTOnlyCorrection;
  PreflateSubModel<2> IrregularLen258Encoding;
};

struct PreflatePredictionModel {
  PreflatePredictionModel();
  ~PreflatePredictionModel();

  void read(const PreflateStatisticsCounter& model, const PreflateModelCodec& cc);
  void setStream(aricoder* codec);
  void readFromStream(const PreflateModelCodec& cc);
  void writeToStream(const PreflateModelCodec& cc);

  bool isEqualTo(const PreflatePredictionModel& m) const;

protected:
  // Blocks
  PreflateBlockPredictionModel block;
  // Tree codes
  PreflateTreeCodePredictionModel treecode;
  // Tokens
  PreflateTokenPredictionModel token;
  iostream* data;
  aricoder* codec;
};

struct PreflatePredictionEncoder : public PreflatePredictionModel {
  void start(const PreflatePredictionModel&, const PreflateParameters&, const unsigned modelId);
  std::vector<uint8_t> end();

  void encodeValue(const unsigned value, const unsigned maxBits) {
    PreflateBaseModel::encodeValue(codec, value, maxBits);
  }

  // Block
  void encodeBlockType(const unsigned type) {
    block.encodeBlockType(type);
  }
  void encodeEOBMisprediction(const bool misprediction) {
    block.encodeEOBMisprediction(misprediction);
  }
  void encodeNonZeroPadding(const bool nonzeropadding) {
    block.encodeNonZeroPadding(nonzeropadding);
  }
  // Tree codes
  void encodeTreeCodeCountMisprediction(const bool misprediction) {
    treecode.encodeTreeCodeCountMisprediction(misprediction);
  }
  void encodeLiteralCountMisprediction(const bool misprediction) {
    treecode.encodeLiteralCountMisprediction(misprediction);
  }
  void encodeDistanceCountMisprediction(const bool misprediction) {
    treecode.encodeDistanceCountMisprediction(misprediction);
  }
  void encodeTreeCodeBitLengthCorrection(const unsigned predval, const unsigned actval) {
    treecode.encodeTreeCodeBitLengthCorrection(predval, actval);
  }
  void encodeLDTypeCorrection(const unsigned predval, const unsigned actval) {
    treecode.encodeLDTypeCorrection(predval, actval);
  }
  void encodeRepeatCountCorrection(const unsigned predval, const unsigned actval, unsigned ldtype) {
    treecode.encodeRepeatCountCorrection(predval, actval, ldtype);
  }
  void encodeLDBitLengthCorrection(const unsigned predval, const unsigned actval) {
    treecode.encodeLDBitLengthCorrection(predval, actval);
  }
  // Token
  void encodeLiteralPredictionWrong(const bool misprediction) {
    token.encodeLiteralPredictionWrong(misprediction);
  }
  void encodeReferencePredictionWrong(const bool misprediction) {
    token.encodeReferencePredictionWrong(misprediction);
  }
  void encodeLenCorrection(const unsigned predval, const unsigned actval) {
    token.encodeLenCorrection(predval, actval);
  }
  void encodeDistOnlyCorrection(const unsigned hops) {
    token.encodeDistOnlyCorrection(hops);
  }
  void encodeDistAfterLenCorrection(const unsigned hops) {
    token.encodeDistAfterLenCorrection(hops);
  }
  void encodeIrregularLen258(const bool irregular) {
    token.encodeIrregularLen258(irregular);
  }

  const PreflateParameters& parameters() const {
    return params;
  }

  unsigned modelId() const {
    return modelid;
  }

private:
  PreflateParameters  params;
  unsigned modelid;
};

struct PreflatePredictionDecoder : public PreflatePredictionModel {
  void start(const PreflatePredictionModel&, const PreflateParameters&, 
             const std::vector<uint8_t>&, size_t off0, size_t size);
  void end();

  unsigned decodeValue(const unsigned maxBits) {
    return PreflateBaseModel::decodeValue(codec, maxBits);
  }
  // Block
  unsigned decodeBlockType() {
    return block.decodeBlockType();
  }
  bool decodeEOBMisprediction() {
    return block.decodeEOBMisprediction();
  }
  bool decodeNonZeroPadding() {
    return block.decodeNonZeroPadding();
  }
  // Tree codes
  bool decodeTreeCodeCountMisprediction() {
    return treecode.decodeTreeCodeCountMisprediction();
  }
  bool decodeLiteralCountMisprediction() {
    return treecode.decodeLiteralCountMisprediction();
  }
  bool decodeDistanceCountMisprediction() {
    return treecode.decodeDistanceCountMisprediction();
  }
  int decodeTreeCodeBitLengthCorrection(unsigned predval) {
    return treecode.decodeTreeCodeBitLengthCorrection(predval);
  }
  unsigned decodeLDTypeCorrection(unsigned predtype) {
    return treecode.decodeLDTypeCorrection(predtype);
  }
  unsigned decodeRepeatCountCorrection(const unsigned predval, const unsigned ldtype) {
    return treecode.decodeRepeatCountCorrection(predval, ldtype);
  }
  unsigned decodeLDBitLengthCorrection(unsigned predval) {
    return treecode.decodeLDBitLengthCorrection(predval);
  }
  // Token
  bool decodeLiteralPredictionWrong() {
    return token.decodeLiteralPredictionWrong();
  }
  bool decodeReferencePredictionWrong() {
    return token.decodeReferencePredictionWrong();
  }
  int decodeLenCorrection(const unsigned predval) {
    return token.decodeLenCorrection(predval);
  }
  unsigned decodeDistOnlyCorrection() {
    return token.decodeDistOnlyCorrection();
  }
  unsigned decodeDistAfterLenCorrection() {
    return token.decodeDistAfterLenCorrection();
  }
  bool decodeIrregularLen258() {
    return token.decodeIrregularLen258();
  }

private:
  iostream* data;
  PreflateParameters  params;
};

struct PreflateMetaEncoder {
  PreflateMetaEncoder();
  ~PreflateMetaEncoder();

  bool error() const {
    return inError;
  }
  unsigned addModel(const PreflateStatisticsCounter&, const PreflateParameters&);

  bool beginMetaBlockWithModel(PreflatePredictionEncoder&, const unsigned modelId);
  bool endMetaBlock(PreflatePredictionEncoder&, const size_t uncompressed);
  std::vector<unsigned char> finish();

private:
  struct modelType {
    unsigned writtenId;
    PreflateStatisticsCounter counter;
    PreflatePredictionModel model;
    PreflateParameters params;
    PreflateModelCodec mcodec;
  };
  struct metaBlockInfo {
    unsigned modelId;
    size_t reconSize;
    size_t uncompressedSize;
  };

  bool inError;
  bool inBlock;
  std::vector<modelType> modelList;
  std::vector<metaBlockInfo> blockList;
  std::vector<uint8_t> reconData;
};

struct PreflateMetaDecoder {
  PreflateMetaDecoder(const std::vector<uint8_t>& reconData, const std::vector<uint8_t>& uncompressed);
  ~PreflateMetaDecoder();

  bool error() const {
    return inError;
  }
  size_t metaBlockCount() const {
    return blockList.size();
  }
  bool beginMetaBlock(PreflatePredictionDecoder&, PreflateParameters&, const size_t index);
  bool endMetaBlock(PreflatePredictionDecoder&);
  void finish();

private:
  struct modelType {
    PreflatePredictionModel model;
    PreflateParameters params;
    PreflateModelCodec mcodec;
  };
  struct metaBlockInfo {
    unsigned modelId;
    size_t reconStartOfs;
    size_t reconSize;
    uint64_t uncompressedStartOfs;
    uint64_t uncompressedSize;
  };

  bool inError;
  bool inBlock;

  const std::vector<uint8_t>& reconData;
  const std::vector<uint8_t>& uncompressedData;
  std::vector<modelType> modelList;
  std::vector<metaBlockInfo> blockList;
};

bool isEqual(const PreflatePredictionModel&, const PreflatePredictionModel&);

#endif /* PREFLATE_STATISTICAL_CODEC_H */
