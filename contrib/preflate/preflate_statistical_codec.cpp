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
#include "support/array_helper.h"
#include "support/bit_helper.h"
#include <stdint.h>

template <unsigned N>
void PreflateSubModel<N>::build_impl(const unsigned* arr, const unsigned defval, const uint8_t prec) {
  if (N == 0) {
    isDefault = true;
    return;
  }
  for (unsigned i = 0; i < N; ++i) {
    ids[i] = i;
  }
  std::sort(ids, ids + N, [=](unsigned i1, unsigned i2) {
    if (arr[i1] != arr[i2]) {
      return arr[i1] < arr[i2];
    }
    return i1 < i2;
  });
  for (unsigned i = 0; i < N; ++i) {
    bounds[i] = arr[ids[i]];
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
  isDefault = N == 0 || bounds[N] == 0 || (bounds[N - 1] == 0 && ids[N - 1] == defval);

  build_scale_down();
}
template <unsigned N>
void PreflateSubModel<N>::buildDefault(const unsigned defval) {
  if (N == 0) {
    isDefault = true;
    return;
  }
  memset(bounds, 0, N * sizeof(unsigned));
  memset(scaledDownBounds, 0, N * sizeof(unsigned));
  bounds[N] = 0x10000;
  ids[N - 1] = defval;
  rids[defval] = N - 1;
  isDefault = true;
  build_scale_down();
}
template <unsigned N>
void PreflateSubModel<N>::build_scale_down() {
  unsigned boundBits = 0;
  for (unsigned i = 0; i <= N; ++i) {
    boundBits |= bounds[i];
  }
  unsigned zeroJunk = bitTrailingZeroes(boundBits);
  scaleDownBits = (16 - zeroJunk);
  for (unsigned i = 0; i <= N; ++i) {
    scaledDownBounds[i] = bounds[i] >> zeroJunk;
  }

  isFixed = bounds[N - 1] == 0;

/*  for (unsigned i = 0; i <= N; ++i) {
    scaledDownBounds[i] = bounds[i];
  }
  scaleDownBits = 16;*/
}


static void encodeProb(ArithmeticEncoder& codec, const unsigned val) {
  unsigned bits = bitLength(val);
  // encode shift
  codec.encodeBits(bits - 1, 4);
  // and precision
  if (bits >= 5) {
    codec.encodeBits((val >> (bits - 5)) & 0xf, 4);
  } else {
    codec.encodeBits(val & ~(1 << (bits - 1)), bits - 1);
  }
}
static void encodeId(ArithmeticEncoder& codec,
              const unsigned id, const unsigned count) {
  unsigned bits = bitLength(count - 1);
  codec.encodeBits(id, bits);
}
static unsigned decodeProb(ArithmeticDecoder& codec) {
  // encode shift
  unsigned bits = codec.decodeBits(4) + 1;
  // and precision
  if (bits >= 5) {
    return (codec.decodeBits(4) | 0x10) << (bits - 5);
  } else {
    return codec.decodeBits(bits - 1) | (1 << (bits - 1));
  }
}
static unsigned decodeId(ArithmeticDecoder& codec, const unsigned count) {
  unsigned bits = bitLength(count - 1);
  return codec.decodeBits(bits);
}

template <unsigned N>
void PreflateSubModel<N>::write(ArithmeticEncoder& codec, const uint8_t) const {
  unsigned zeros = 0;
  for (unsigned i = 1; i < N; ++i) {
    if (!bounds[i]) {
      ++zeros;
    } else {
      break;
    }
  }
  codec.encodeBits(zeros, bitLength(N - 1));
  // Transmit values
  for (unsigned i = 1 + zeros; i < N; ++i) {
    encodeProb(codec, bounds[i] - bounds[i - 1]);
  }
  // Transmit ids
  for (unsigned i = zeros; i < N; ++i) {
    encodeId(codec, ids[i], N);
  }
}
template <unsigned N>
void PreflateSubModel<N>::read(ArithmeticDecoder& codec, const uint8_t) {
  unsigned zeros = codec.decodeBits(bitLength(N - 1));
  memset(bounds, 0, sizeof(bounds));
  // Transmit values
  for (unsigned i = 1 + zeros; i < N; ++i) {
    bounds[i] = decodeProb(codec) + bounds[i - 1];
  }
  bounds[N] = 1 << 16;
  // Transmit ids
  for (unsigned i = zeros; i < N; ++i) {
    ids[i] = decodeId(codec, N);
    rids[ids[i]] = i;
  }
  build_scale_down();
}

template <unsigned NEG, unsigned POS>
void PreflateCorrectionSubModel<NEG, POS>::build_impl(const unsigned* arr, const int defval, const uint8_t prec) {
  unsigned signArr[3] = {arr[NEG], sumArray(arr + NEG + 1, POS), sumArray(arr, NEG)};
  sign.build_impl(signArr, defval == 0 ? 0 : (defval > 0 ? 1 : 2), prec);
  unsigned posArr[POS + 1];
  for (unsigned i = 0; i < POS; ++i) {
    posArr[i] = arr[NEG + 1 + i];
  }
  pos.build_impl(posArr, defval > 0 && defval <= POS ? defval - 1 : POS - 1, prec);
  unsigned negArr[NEG + 1];
  for (unsigned i = 0; i < NEG; ++i) {
    negArr[i] = arr[NEG - 1 - i];
  }
  neg.build_impl(negArr, -defval > 0 && -defval <= NEG ? -defval - 1 : NEG - 1, prec);
  isDefault = sign.isDefault && pos.isDefault && neg.isDefault;
}

template <unsigned NEG, unsigned POS>
void PreflateCorrectionSubModel<NEG, POS>::buildDefault(const unsigned defval) {
  sign.buildDefault(defval == 0 ? 0 : (defval > 0 ? 1 : 2));
  pos.buildDefault(defval > 0 && defval <= POS ? defval - 1 : POS - 1);
  neg.buildDefault(-defval > 0 && -defval <= NEG ? -defval - 1 : NEG - 1);
  isDefault = sign.isDefault && pos.isDefault && neg.isDefault;
}
template <unsigned NEG, unsigned POS>
void PreflateCorrectionSubModel<NEG, POS>::write(ArithmeticEncoder& codec, const uint8_t prec) const {
  sign.write(codec, prec);
  if (POS > 0) {
    pos.write(codec, prec);
  }
  if (NEG > 0) {
    neg.write(codec, prec);
  }
}
template <unsigned NEG, unsigned POS>
void PreflateCorrectionSubModel<NEG, POS>::read(ArithmeticDecoder& codec, const uint8_t prec) {
  sign.read(codec, prec);
  if (POS > 0) {
    pos.read(codec, prec);
  }
  if (NEG > 0) {
    neg.read(codec, prec);
  }
}

// -------------------------------------

PreflateBaseModel::PreflateBaseModel() 
  : encoder(nullptr), decoder(nullptr) {}

void PreflateBaseModel::setEncoderStream(ArithmeticEncoder* codec_) {
  encoder = codec_;
}
void PreflateBaseModel::setDecoderStream(ArithmeticDecoder* codec_) {
  decoder = codec_;
}

template <unsigned N>
void PreflateBaseModel::readSubModel(PreflateSubModel<N>& sm, const bool isFullDef, const PreflateModelCodec& cc,
                  const unsigned defVal, const uint8_t prec) {
  if (isFullDef || cc.nonDefaultValue.decode(*decoder) == 0) {
    sm.buildDefault(defVal);
  } else {
    sm.read(*decoder, prec);
  }
}

template <unsigned N, unsigned M>
void PreflateBaseModel::readSubModel(PreflateCorrectionSubModel<N, M>& sm, const bool isFullDef, const PreflateModelCodec& cc,
                  const unsigned defVal, const uint8_t prec) {
  if (isFullDef || cc.nonDefaultValue.decode(*decoder) == 0) {
    sm.buildDefault(defVal);
  } else {
    sm.read(*decoder, prec);
  }
}

template <unsigned N>
void PreflateBaseModel::writeSubModel(const PreflateSubModel<N>& sm, const bool isFullDef, const PreflateModelCodec& cc,
                   const unsigned defVal, const uint8_t prec) {
  if (isFullDef) {
    return;
  }
  bool ndef = !sm.isDefault;
  cc.nonDefaultValue.encode(*encoder, ndef);
  if (ndef) {
    sm.write(*encoder, prec);
  }
}

template <unsigned N, unsigned M>
void PreflateBaseModel::writeSubModel(const PreflateCorrectionSubModel<N, M>& sm, const bool isFullDef, const PreflateModelCodec& cc,
                   const unsigned defVal, const uint8_t prec) {
  if (isFullDef) {
    return;
  }
  bool ndef = !sm.isDefault;
  cc.nonDefaultValue.encode(*encoder, ndef);
  if (ndef) {
    sm.write(*encoder, prec);
  }
}

void PreflateBlockPredictionModel::read(const PreflateStatisticsCounter::BlockPrediction& blockModel, const PreflateModelCodec& cc) {
  blockType.build(blockModel.blockType, PreflateTokenBlock::DYNAMIC_HUFF, cc.MBprecision);
  EOBMisprediction.build(blockModel.EOBMisprediction, 0, cc.MBprecision);
  nonZeroPadding.build(blockModel.nonZeroPadding, 0, cc.MBprecisionP1);
}
void PreflateBlockPredictionModel::readFromStream(const PreflateModelCodec& cc) {
  readSubModel(blockType, cc.blockFullDefault, cc, PreflateTokenBlock::DYNAMIC_HUFF, cc.MBprecision);
  readSubModel(EOBMisprediction, cc.blockFullDefault, cc, 0, cc.MBprecision);
  readSubModel(nonZeroPadding, cc.blockFullDefault, cc, 0, cc.MBprecisionP1);
}
void PreflateBlockPredictionModel::writeToStream(const PreflateModelCodec& cc) {
  writeSubModel(blockType, cc.blockFullDefault, cc, PreflateTokenBlock::DYNAMIC_HUFF, cc.MBprecision);
  writeSubModel(EOBMisprediction, cc.blockFullDefault, cc, 0, cc.MBprecision);
  writeSubModel(nonZeroPadding, cc.blockFullDefault, cc, 0, cc.MBprecisionP1);
}

void PreflateTreeCodePredictionModel::read(const PreflateStatisticsCounter::TreeCodePrediction& treecodeModel, const PreflateModelCodec& cc) {
  TCCountMisprediction.build(treecodeModel.TCCountMisprediction, 0, cc.MBprecision);
  LCountMisprediction.build(treecodeModel.LCountMisprediction, 0, cc.MBprecision);
  DCountMisprediction.build(treecodeModel.DCountMisprediction, 0, cc.MBprecision);
  for (unsigned i = 0; i < 4; ++i) {
    LDTypeMisprediction[i].build(treecodeModel.LDTypeMisprediction[i], 0);
  }
  LDTypeReplacementBase.build(treecodeModel.LDTypeReplacement, 0);
  TCBitlengthCorrection.build(treecodeModel.TCBitlengthCorrection, 0);
  LDBitlengthCorrection.build(treecodeModel.LDBitlengthCorrection, 0);
  LDRepeatCountCorrection.build(treecodeModel.LDRepeatCountCorrection, 0);

  deriveLDTypeReplacement();
}
void PreflateTreeCodePredictionModel::readFromStream(const PreflateModelCodec& cc) {
  readSubModel(TCCountMisprediction, cc.treecodeFullDefault, cc, 0, cc.MBprecision);
  readSubModel(LCountMisprediction, cc.treecodeFullDefault, cc, 0, cc.MBprecision);
  readSubModel(DCountMisprediction, cc.treecodeFullDefault, cc, 0, cc.MBprecision);
  for (unsigned i = 0; i < 4; ++i) {
    readSubModel(LDTypeMisprediction[i], cc.treecodeFullDefault, cc, 0);
  }
  readSubModel(LDTypeReplacementBase, cc.treecodeFullDefault, cc, 0);
  readSubModel(TCBitlengthCorrection, cc.treecodeFullDefault, cc, 0);
  readSubModel(LDBitlengthCorrection, cc.treecodeFullDefault, cc, 0);
  readSubModel(LDRepeatCountCorrection, cc.treecodeFullDefault, cc, 0);

  deriveLDTypeReplacement();
}
void PreflateTreeCodePredictionModel::writeToStream(const PreflateModelCodec& cc) {
  writeSubModel(TCCountMisprediction, cc.treecodeFullDefault, cc, 0, cc.MBprecision);
  writeSubModel(LCountMisprediction, cc.treecodeFullDefault, cc, 0, cc.MBprecision);
  writeSubModel(DCountMisprediction, cc.treecodeFullDefault, cc, 0, cc.MBprecision);
  for (unsigned i = 0; i < 4; ++i) {
    writeSubModel(LDTypeMisprediction[i], cc.treecodeFullDefault, cc, 0);
  }
  writeSubModel(LDTypeReplacementBase, cc.treecodeFullDefault, cc, 0);
  writeSubModel(TCBitlengthCorrection, cc.treecodeFullDefault, cc, 0);
  writeSubModel(LDBitlengthCorrection, cc.treecodeFullDefault, cc, 0);
  writeSubModel(LDRepeatCountCorrection, cc.treecodeFullDefault, cc, 0);
}
void PreflateTreeCodePredictionModel::deriveLDTypeReplacement() {
  unsigned arr[4], arr_mp[2], miss[4], hit[4], sumhit;
  LDTypeReplacementBase.extract(arr);
  for (unsigned i = 0; i < 4; ++i) {
    LDTypeMisprediction[i].extract(arr_mp);
    if (arr_mp[1] == 0) {
      DerivedLDTypeReplacement[i].buildDefault(i);
    } else {
      if (arr_mp[0] == 0) {
        arr_mp[1] = 1;
      }
      sumhit = 0;
      for (unsigned j = 0; j < 4; ++j) {
        hit[j] = arr[j] * arr_mp[0];
        miss[j] = arr[j] * arr_mp[1];
        sumhit += hit[j];
      }
      miss[i] = sumhit - hit[i];
      // Avoid the sum of all entries to exceed 32bit
      for (unsigned j = 0; j < 4; ++j) {
        if (miss[j] > 0 && miss[j] < 16) {
          miss[j] = 1;
        } else {
          miss[j] >>= 4;
        }
      }
      DerivedLDTypeReplacement[i].build(miss, i);
    }
  }
}


void PreflateTokenPredictionModel::read(const PreflateStatisticsCounter::TokenPrediction& tokenModel, const PreflateModelCodec& cc) {
  LITMisprediction.build(tokenModel.LITMisprediction, 0);
  REFMisprediction.build(tokenModel.REFMisprediction, 0);
  LENCorrection.build(tokenModel.LENCorrection, 0);
  DISTAfterLenCorrection.build(tokenModel.DISTAfterLenCorrection, 0);
  DISTOnlyCorrection.build(tokenModel.DISTOnlyCorrection, 0);
  IrregularLen258Encoding.build(tokenModel.LEN258IrregularEncoding, 0);
}
void PreflateTokenPredictionModel::readFromStream(const PreflateModelCodec& cc) {
  readSubModel(LITMisprediction, cc.tokenFullDefault, cc, 0);
  readSubModel(REFMisprediction, cc.tokenFullDefault, cc, 0);
  readSubModel(LENCorrection, cc.tokenFullDefault, cc, 0);
  readSubModel(DISTAfterLenCorrection, cc.tokenFullDefault, cc, 0);
  readSubModel(DISTOnlyCorrection, cc.tokenFullDefault, cc, 0);
  readSubModel(IrregularLen258Encoding, cc.tokenFullDefault, cc, 0);
}
void PreflateTokenPredictionModel::writeToStream(const PreflateModelCodec& cc) {
  writeSubModel(LITMisprediction, cc.tokenFullDefault, cc, 0);
  writeSubModel(REFMisprediction, cc.tokenFullDefault, cc, 0);
  writeSubModel(LENCorrection, cc.tokenFullDefault, cc, 0);
  writeSubModel(DISTAfterLenCorrection, cc.tokenFullDefault, cc, 0);
  writeSubModel(DISTOnlyCorrection, cc.tokenFullDefault, cc, 0);
  writeSubModel(IrregularLen258Encoding, cc.tokenFullDefault, cc, 0);
}


PreflatePredictionModel::PreflatePredictionModel() {}
PreflatePredictionModel::~PreflatePredictionModel() {}

void PreflatePredictionModel::read(const PreflateStatisticsCounter& model, const PreflateModelCodec& cc) {
  block.read(model.block, cc);
  treecode.read(model.treecode, cc);
  token.read(model.token, cc);
}
void PreflatePredictionModel::setEncoderStream(ArithmeticEncoder* codec) {
  block.setEncoderStream(codec);
  treecode.setEncoderStream(codec);
  token.setEncoderStream(codec);
}
void PreflatePredictionModel::setDecoderStream(ArithmeticDecoder* codec) {
  block.setDecoderStream(codec);
  treecode.setDecoderStream(codec);
  token.setDecoderStream(codec);
}
void PreflatePredictionModel::readFromStream(const PreflateModelCodec& cc) {
  block.readFromStream(cc);
  treecode.readFromStream(cc);
  token.readFromStream(cc);
}
void PreflatePredictionModel::writeToStream(const PreflateModelCodec& cc) {
  block.writeToStream(cc);
  treecode.writeToStream(cc);
  token.writeToStream(cc);
}

// ------------------------------------

PreflateModelCodec::PreflateModelCodec() {}
void PreflateModelCodec::initDefault() {
  blockFullDefault = true;
  treecodeFullDefault = true;
  tokenFullDefault = true;
  totalModels = 0;
  defaultingModels = 0;

  unsigned arr[2] = {1, 0};
  nonDefaultValue.build(arr, 0);

  MBprecision = 16;
  MBprecisionP1 = 16;
}

void PreflateModelCodec::read(const PreflateStatisticsCounter& m) {
  totalModels = 0;
  defaultingModels = 0;
  unsigned total_block = m.block.totalModels();
  unsigned defaulting_block = m.block.checkDefaultModels();
  blockFullDefault = total_block == defaulting_block;
  if (!blockFullDefault) {
    totalModels += total_block;
    defaultingModels += defaulting_block;
  }

  unsigned total_tree = m.treecode.totalModels();
  unsigned defaulting_tree = m.treecode.checkDefaultModels();
  treecodeFullDefault = total_tree == defaulting_tree;
  if (!treecodeFullDefault) {
    totalModels += total_tree;
    defaultingModels += defaulting_tree;
  }

  unsigned total_token = m.token.totalModels();
  unsigned defaulting_token = m.token.checkDefaultModels();
  tokenFullDefault = total_token == defaulting_token;
  if (!tokenFullDefault) {
    totalModels += total_token;
    defaultingModels += defaulting_token;
  }

  if (totalModels > 0) {
    unsigned arr[2] = {defaultingModels, totalModels - defaultingModels};
    nonDefaultValue.build(arr, 0);
  }
  MBprecision   = 16;
  MBprecisionP1 = 16;
}

void PreflateModelCodec::readFromStream(ArithmeticDecoder& codec) {
  blockFullDefault = codec.decodeBits(1); 
  treecodeFullDefault = codec.decodeBits(1);
  tokenFullDefault = codec.decodeBits(1);
  totalModels = 0;
  if (!blockFullDefault) {
    totalModels += PreflateStatisticsCounter::BlockPrediction::totalModels();
  }
  if (!treecodeFullDefault) {
    totalModels += PreflateStatisticsCounter::TreeCodePrediction::totalModels();
  }
  if (!tokenFullDefault) {
    totalModels += PreflateStatisticsCounter::TokenPrediction::totalModels();
  }
  defaultingModels = PreflateBaseModel::decodeValue(codec, bitLength(totalModels));

  if (totalModels) {
    unsigned arr[2] = {defaultingModels, totalModels - defaultingModels};
    nonDefaultValue.build(arr, 0);
  }
  MBprecision = 16;
  MBprecisionP1 = 16;
}
void PreflateModelCodec::writeToStream(ArithmeticEncoder& codec) {
  codec.encodeBits(blockFullDefault, 1);
  codec.encodeBits(treecodeFullDefault, 1);
  codec.encodeBits(tokenFullDefault, 1);
  codec.encodeBits(defaultingModels, bitLength(totalModels));
}

// ------------------------------------

PreflatePredictionEncoder::PreflatePredictionEncoder() 
  : storage(nullptr)
  , bos(nullptr)
  , encoder(nullptr)
{}

void PreflatePredictionEncoder::start(const PreflatePredictionModel& model_, const PreflateParameters& params_,
                                      const unsigned modelId_) {
  PreflatePredictionModel::operator =(model_);
  params = params_;
  modelid = modelId_;

  storage = new MemStream;
  bos = new BitOutputStream(*storage);
  encoder = new ArithmeticEncoder(*bos);
  setEncoderStream(encoder);
}
std::vector<uint8_t> PreflatePredictionEncoder::end() {
  setEncoderStream(nullptr);
  encoder->flush();
  delete encoder;

  bos->flush();
  delete bos;

  std::vector<unsigned char> result = storage->extractData();
  delete storage;
  return result;
}

PreflatePredictionDecoder::PreflatePredictionDecoder()
  : storage(nullptr)
  , bis(nullptr)
  , decoder(nullptr) {}

void PreflatePredictionDecoder::start(const PreflatePredictionModel& model_, const PreflateParameters& params_,
                                      const std::vector<uint8_t>& storage_, size_t off0, size_t size) {
  PreflatePredictionModel::operator =(model_);
  params = params_;
  storage = new MemStream(storage_, off0, size);
  bis = new BitInputStream(*storage);
  decoder = new ArithmeticDecoder(*bis);
  setDecoderStream(decoder);
}
void PreflatePredictionDecoder::end() {
  setDecoderStream(nullptr);
  delete decoder;
  delete bis;
  delete storage;
  decoder = nullptr;
  bis = nullptr;
  storage = nullptr;
}

// ------------------------------------

PreflateMetaEncoder::PreflateMetaEncoder()
  : inError(false) {
}
PreflateMetaEncoder::~PreflateMetaEncoder() {}

unsigned PreflateMetaEncoder::addModel(const PreflateStatisticsCounter& counter, const PreflateParameters& params) {
  unsigned modelId = modelList.size();
  modelType m;
  m.counter = counter;
  m.mcodec.read(counter);
  m.model.read(counter, m.mcodec);
  m.params = params;
  m.writtenId = 0;
  modelList.push_back(m);
  return modelId;
}

bool PreflateMetaEncoder::beginMetaBlockWithModel(PreflatePredictionEncoder& encoder, const unsigned modelId) {
  if (modelId >= modelList.size()) {
    return false;
  }
  encoder.start(modelList[modelId].model, modelList[modelId].params, modelId);
  return true;
}
bool PreflateMetaEncoder::endMetaBlock(PreflatePredictionEncoder& encoder, const size_t uncompressed) {
  if (encoder.modelId() >= modelList.size()) {
    return false;
  }
  metaBlockInfo m;
  std::vector<uint8_t> result = encoder.end();
  m.modelId = encoder.modelId();
  m.reconSize = result.size();
  m.uncompressedSize = uncompressed;
  blockList.push_back(m);
  reconData.insert(reconData.end(), result.begin(), result.end());
  return true;
}
std::vector<unsigned char> PreflateMetaEncoder::finish() {
  MemStream mem;
  BitOutputStream bos(mem);
  bos.put(0, 1); // no extension used
  bos.put(blockList.size() > 1, 1); // 1 or more meta blocks
  if (blockList.size() > 1) {
    bos.putVLI(blockList.size() - 2);
  }
  enum Mode {
    CREATE_NEW_MODEL /*, REUSE_LAST_MODEL, REUSE_PREVIOUS_MODEL*/
  };
  for (unsigned i = 0, n = blockList.size(); i < n; ++i) {
    const metaBlockInfo& mb = blockList[i];
    Mode mode = CREATE_NEW_MODEL;
    
    if (i > 0) {
      bos.put(3, 2); // create new model
    }

    switch (mode) {
    case CREATE_NEW_MODEL:
    {
      modelType& mt = modelList[mb.modelId];
      bool perfectZLIB = mt.mcodec.blockFullDefault && mt.mcodec.treecodeFullDefault && mt.mcodec.tokenFullDefault
        && mt.params.zlibCompatible;
      bos.put(!perfectZLIB, 1); // perfect zlib model
      bos.put(mt.params.compLevel, 4);
      bos.put(mt.params.memLevel, 4);
      bos.put(mt.params.windowBits - 8, 3);
      if (!perfectZLIB) {
        bos.put(mt.params.zlibCompatible, 1);
        if (!mt.params.zlibCompatible) {
          bos.put(mt.params.veryFarMatchesDetected, 1);
          bos.put(mt.params.matchesToStartDetected, 1);
        }
        bos.put(mt.params.log2OfMaxChainDepthM1, 4);
        MemStream tmp_data;
        {
          BitOutputStream tmp_bos(tmp_data);
          ArithmeticEncoder tmp_codec(tmp_bos);
          mt.mcodec.writeToStream(tmp_codec);
          mt.model.setEncoderStream(&tmp_codec);
          mt.model.writeToStream(mt.mcodec);
          mt.model.setEncoderStream(nullptr);
          tmp_codec.flush();
          tmp_bos.flush();
        }
        std::vector<uint8_t> tmp_res = tmp_data.extractData();
        // write length (vli) and model data
        bos.putVLI(tmp_res.size());
        bos.putBytes(tmp_res.data(), tmp_res.size());
      }
      break;
    }
    }
    // for the last block, the size of the reconstruction data and processed uncompressed data
    // is implicitly going to end of stream
    // -------------------
    if (i != n - 1) {
      bos.putVLI(mb.reconSize);
      bos.putVLI(mb.uncompressedSize);
    }
  }
  bos.flush();
  std::vector<uint8_t> result = mem.extractData();
  result.insert(result.end(), reconData.begin(), reconData.end());
  return result;
}

PreflateMetaDecoder::PreflateMetaDecoder(const std::vector<uint8_t>& reconData_, const uint64_t uncompressedSize_)
  : inError(false)
  , reconData(reconData_)
  , uncompressedSize(uncompressedSize_) {
  if (reconData.size() == 0) {
    inError = true;
    return;
  }
  MemStream mem(reconData);
  BitInputStream bis(mem);
  bool extension = bis.get(1);
  if (extension) {
    inError = true;
    return;
  }
  bool singleBlock = bis.get(1) == 0;
  size_t blockCount;
  if (singleBlock) {
    blockCount = 1;
  } else {
    blockCount = 2 + bis.getVLI();
  }
  enum Mode {
    CREATE_NEW_MODEL /*, REUSE_LAST_MODEL, REUSE_PREVIOUS_MODEL*/
  };
  for (size_t i = 0; i < blockCount; ++i) {
    metaBlockInfo mb;
    Mode mode = CREATE_NEW_MODEL;

    if (i > 0) {
      if (bis.get(2) != 3) { // must create new model for the moment
        inError = true;
        return;
      }
    }

    switch (mode) {
    case CREATE_NEW_MODEL:
    {
      modelType mt;
      memset(&mt, 0, sizeof(mt));
      bool perfectZLIB = bis.get(1) == 0;
      mt.params.compLevel = bis.get(4);
      mt.params.memLevel = bis.get(4);
      mt.params.windowBits = bis.get(3) + 8;
      if (perfectZLIB) {
        mt.params.zlibCompatible = true;
        mt.mcodec.blockFullDefault = true;
        mt.mcodec.treecodeFullDefault = true;
        mt.mcodec.tokenFullDefault = true;
        mt.model.readFromStream(mt.mcodec); // initialize with default model
      } else {
        mt.params.zlibCompatible = bis.get(1);
        if (!mt.params.zlibCompatible) {
          mt.params.veryFarMatchesDetected = bis.get(1);
          mt.params.matchesToStartDetected = bis.get(1);
        }
        mt.params.log2OfMaxChainDepthM1 = bis.get(4);
        // read length (vli) and model data
        size_t res_size = bis.getVLI();
        // interpret model data
        {
          MemStream tmp_mem;
          bis.copyBytesTo(tmp_mem, res_size);
          tmp_mem.seek(0);
          BitInputStream tmp_bis(tmp_mem);
          ArithmeticDecoder tmp_codec(tmp_bis);
          mt.mcodec.readFromStream(tmp_codec);
          mt.model.setDecoderStream(&tmp_codec);
          mt.model.readFromStream(mt.mcodec);
          mt.model.setDecoderStream(nullptr);
        }
      }
      mb.modelId = modelList.size();
      modelList.push_back(mt);
      break;
    }
    }
    // for the last block, the size of the reconstruction data and processed uncompressed data
    // is implicitly going to end of stream
    // -------------------
    if (i != blockCount - 1) {
      mb.reconSize = bis.getVLI();
      mb.uncompressedSize = bis.getVLI();
    }
    blockList.push_back(mb);
  }
  bis.skipToByte();

  size_t reconStart = bis.bitPos() >> 3;
  uint64_t uncStart = 0;
  for (size_t i = 0; i < blockCount; ++i) {
    blockList[i].reconStartOfs = reconStart;
    blockList[i].uncompressedStartOfs = uncStart;
    if (i != blockCount - 1) {
      reconStart += blockList[i].reconSize;
      uncStart += blockList[i].uncompressedSize;
      if (reconStart > reconData.size() || uncStart > uncompressedSize) {
        inError = true;
        return;
      }
    } else {
      blockList[i].reconSize = reconData.size() - blockList[i].reconStartOfs;
      blockList[i].uncompressedSize = uncompressedSize - blockList[i].uncompressedStartOfs;
    }
  }
}
PreflateMetaDecoder::~PreflateMetaDecoder() {}

bool PreflateMetaDecoder::beginMetaBlock(PreflatePredictionDecoder& decoder, PreflateParameters& params, const size_t index) {
  if (index >= blockList.size()) {
    return false;
  }
  const auto& mb = blockList[index];
  if (mb.modelId >= modelList.size()) {
    return false;
  }
  const auto& model = modelList[mb.modelId];
  params = model.params;
  decoder.start(model.model, model.params, reconData, mb.reconStartOfs, mb.reconSize);
  return true;
}
bool PreflateMetaDecoder::endMetaBlock(PreflatePredictionDecoder& decoder) {
  decoder.end();
  return true;
}
void PreflateMetaDecoder::finish() {}
