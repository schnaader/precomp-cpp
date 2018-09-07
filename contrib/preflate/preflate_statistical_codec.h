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
#include "support/arithmetic_coder.h"
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
  void read(ArithmeticDecoder&, const uint8_t);
  void write(ArithmeticEncoder&, const uint8_t) const;
  void encode(ArithmeticEncoder& codec, const unsigned item) const {
    if (!isFixed) {
      size_t idx = rids[item];
      codec.encodeShiftScale(scaleDownBits, scaledDownBounds[idx], scaledDownBounds[idx + 1]);
    }
  }
  unsigned decode(ArithmeticDecoder& codec) const {
    if (isFixed) {
      return ids[N - 1];
    }
    unsigned val = codec.decodeShiftScale(scaleDownBits, scaledDownBounds, N);
    return ids[val];
  }
  bool isEqualTo(const PreflateSubModel<N>& m) const;

  unsigned bounds[N + 1];
  unsigned scaledDownBounds[N + 1];
  unsigned short ids[N + 1], rids[N + 1];
  uint8_t scaleDownBits;
  bool isDefault, isFixed;

private:
  void build_impl(const unsigned* arr, const unsigned defval, const uint8_t prec);
  void build_scale_down();
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
  void read(ArithmeticDecoder&, const uint8_t);
  void write(ArithmeticEncoder&, const uint8_t) const;
  void encode(ArithmeticEncoder& codec, const unsigned actvalue,
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
        codec.encodeBits(diff - POS, bitLength(maxvalue - POS - refvalue));
      } else {
        pos.encode(codec, diff - 1);
      }
    } else {
      sign.encode(codec, 2);
      if (-diff >= (int)NEG) {
        neg.encode(codec, NEG - 1);
        codec.encodeBits(-diff - NEG, bitLength(refvalue - NEG - minvalue));
      } else {
        neg.encode(codec, -diff - 1);
      }
    }
  }
  unsigned decode(ArithmeticDecoder& codec,
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
        return refvalue + codec.decodeBits(bitLength(maxvalue - POS - refvalue)) + POS;
      } else {
        return refvalue + diff + 1;
      }
    } else {
      int diff = neg.decode(codec);
      if (diff >= (int)(NEG - 1)) {
        return refvalue - codec.decodeBits(bitLength(refvalue - NEG - minvalue)) - NEG;
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
  void readFromStream(ArithmeticDecoder&);
  void writeToStream(ArithmeticEncoder&);
};

struct PreflateBaseModel {
public:
  PreflateBaseModel();
  void setEncoderStream(ArithmeticEncoder*);
  void setDecoderStream(ArithmeticDecoder*);

  static void encodeValue(ArithmeticEncoder& codec, const unsigned value, const unsigned maxBits) {
#ifdef _DEBUG
    _ASSERT(value < (1 << maxBits));
#endif
    return codec.encodeBits(value, maxBits);
  }
  void encodeValue(const unsigned value, const unsigned maxBits) {
    encodeValue(*encoder, value, maxBits);
  }
  static unsigned decodeValue(ArithmeticDecoder& codec, const unsigned maxBits) {
    return codec.decodeBits(maxBits);
  }
  unsigned decodeValue(const unsigned maxBits) {
    return decodeValue(*decoder, maxBits);
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

  ArithmeticEncoder* encoder;
  ArithmeticDecoder* decoder;
};

struct PreflateBlockPredictionModel : public PreflateBaseModel {
public:
  void read(const PreflateStatisticsCounter::BlockPrediction&, const PreflateModelCodec&);
  void readFromStream(const PreflateModelCodec&);
  void writeToStream(const PreflateModelCodec&);

  unsigned decodeBlockType() {
    return blockType.decode(*decoder);
  }
  bool decodeEOBMisprediction() {
    return EOBMisprediction.decode(*decoder);
  }
  bool decodeNonZeroPadding() {
    return nonZeroPadding.decode(*decoder);
  }

  void encodeBlockType(const unsigned type) {
    blockType.encode(*encoder, type);
  }
  void encodeEOBMisprediction(const bool misprediction) {
    EOBMisprediction.encode(*encoder, misprediction);
  }
  void encodeNonZeroPadding(const bool nonzeropadding) {
    nonZeroPadding.encode(*encoder, nonzeropadding);
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
    return TCCountMisprediction.decode(*decoder);
  }
  bool decodeLiteralCountMisprediction() {
    return LCountMisprediction.decode(*decoder);
  }
  bool decodeDistanceCountMisprediction() {
    return DCountMisprediction.decode(*decoder);
  }
  int decodeTreeCodeBitLengthCorrection(unsigned predval) {
    return TCBitlengthCorrection.decode(*decoder, predval, 0, 7);
  }
  unsigned decodeLDTypeCorrection(unsigned predtype) {
    return DerivedLDTypeReplacement[predtype].decode(*decoder);
  }
  unsigned decodeRepeatCountCorrection(const unsigned predval, const unsigned ldtype) {
    static const uint8_t minVal[4] = {0, 3, 3, 11};
    static const uint8_t lenVal[4] = {0, 3, 7, 127};
    return LDRepeatCountCorrection.decode(*decoder, predval, minVal[ldtype], minVal[ldtype] + lenVal[ldtype]);
  }
  int decodeLDBitLengthCorrection(unsigned predval) {
    return LDBitlengthCorrection.decode(*decoder, predval, 0, 15);
  }

  void encodeTreeCodeCountMisprediction(const bool misprediction) {
    TCCountMisprediction.encode(*encoder, misprediction);
  }
  void encodeLiteralCountMisprediction(const bool misprediction) {
    LCountMisprediction.encode(*encoder, misprediction);
  }
  void encodeDistanceCountMisprediction(const bool misprediction) {
    DCountMisprediction.encode(*encoder, misprediction);
  }
  void encodeTreeCodeBitLengthCorrection(const unsigned predval, const unsigned actval) {
    TCBitlengthCorrection.encode(*encoder, actval, predval, 0, 7);
  }
  void encodeLDTypeCorrection(const unsigned predval, const unsigned actval) {
    DerivedLDTypeReplacement[predval].encode(*encoder, actval);
  }
  void encodeRepeatCountCorrection(const unsigned predval, const unsigned actval, unsigned ldtype) {
    static const uint8_t minVal[4] = {0, 3, 3, 11};
    static const uint8_t lenVal[4] = {0, 3, 7, 127};
    LDRepeatCountCorrection.encode(*encoder, actval, predval, minVal[ldtype], minVal[ldtype] + lenVal[ldtype]);
  }
  void encodeLDBitLengthCorrection(const unsigned predval, const unsigned actval) {
    LDBitlengthCorrection.encode(*encoder, actval, predval, 0, 15);
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
    return LITMisprediction.decode(*decoder);
  }
  bool decodeReferencePredictionWrong() {
    return REFMisprediction.decode(*decoder);
  }
  int decodeLenCorrection(const unsigned predval) {
    return LENCorrection.decode(*decoder, predval, 3, 258);
  }
  unsigned decodeDistOnlyCorrection() {
    return DISTOnlyCorrection.decode(*decoder, 0, 0, 32767);
  }
  unsigned decodeDistAfterLenCorrection() {
    return DISTAfterLenCorrection.decode(*decoder, 0, 0, 32767);
  }
  bool decodeIrregularLen258() {
    return IrregularLen258Encoding.decode(*decoder);
  }

  void encodeLiteralPredictionWrong(const bool misprediction) {
    LITMisprediction.encode(*encoder, misprediction);
  }
  void encodeReferencePredictionWrong(const bool misprediction) {
    REFMisprediction.encode(*encoder, misprediction);
  }
  void encodeLenCorrection(const unsigned predval, const unsigned actval) {
    LENCorrection.encode(*encoder, actval, predval, 3, 258);
  }
  void encodeDistOnlyCorrection(const unsigned hops) {
    DISTOnlyCorrection.encode(*encoder, hops, 0, 0, 32767);
  }
  void encodeDistAfterLenCorrection(const unsigned hops) {
    DISTAfterLenCorrection.encode(*encoder, hops, 0, 0, 32767);
  }
  void encodeIrregularLen258(const bool irregular) {
    IrregularLen258Encoding.encode(*encoder, irregular);
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
  void setEncoderStream(ArithmeticEncoder* codec);
  void setDecoderStream(ArithmeticDecoder* codec);
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
};

struct PreflatePredictionEncoder : public PreflatePredictionModel {
  PreflatePredictionEncoder();

  void start(const PreflatePredictionModel&, const PreflateParameters&, const unsigned modelId);
  std::vector<uint8_t> end();

  void encodeValue(const unsigned value, const unsigned maxBits) {
    encoder->encodeBits(value, maxBits);
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
  MemStream* storage;
  BitOutputStream* bos;
  ArithmeticEncoder* encoder;
};

struct PreflatePredictionDecoder : public PreflatePredictionModel {
  PreflatePredictionDecoder();
  void start(const PreflatePredictionModel&, const PreflateParameters&, 
             const std::vector<uint8_t>&, size_t off0, size_t size);
  void end();

  unsigned decodeValue(const unsigned maxBits) {
    return decoder->decodeBits(maxBits);
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
  PreflateParameters  params;
  MemStream* storage;
  BitInputStream* bis;
  ArithmeticDecoder* decoder;
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
  PreflateMetaDecoder(const std::vector<uint8_t>& reconData, const uint64_t uncompressedSize);
  ~PreflateMetaDecoder();

  bool error() const {
    return inError;
  }
  size_t metaBlockCount() const {
    return blockList.size();
  }
  uint64_t metaBlockUncompressedStartOfs(const size_t metaBlockId) const {
    return blockList[metaBlockId].uncompressedStartOfs;
  }
  size_t metaBlockUncompressedSize(const size_t metaBlockId) const {
    return blockList[metaBlockId].uncompressedSize;
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

  size_t currentMetaBlockId;
  const std::vector<uint8_t>& reconData;
  const uint64_t uncompressedSize;
  std::vector<modelType> modelList;
  std::vector<metaBlockInfo> blockList;
};

bool isEqual(const PreflatePredictionModel&, const PreflatePredictionModel&);

#endif /* PREFLATE_STATISTICAL_CODEC_H */
