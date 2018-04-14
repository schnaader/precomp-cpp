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

struct PreflateParameters;

class iostream;
class aricoder;

enum PreflateCorrectionType {
  // Block
  CORR_BLOCK_TYPE,
  CORR_EOB_MISPREDICTION,
  // Tokens
  CORR_LIT_MISPREDICTION,
  CORR_REF_MISPREDICTION,
  CORR_LEN_MISPREDICTION,
  CORR_LEN_CORRECTION,
  CORR_DIST_AFTER_LEN_CORRECTION,
  CORR_DIST_ONLY_MISPREDICTION,
  CORR_DIST_ONLY_CORRECTION,
  // Trees
  CORR_L_COUNT_MISPREDICTION,
  CORR_D_COUNT_MISPREDICTION,
  CORR_LD_TYPE_MISPREDICTION,
  CORR_LD_TYPE_REPLACEMENT,
  CORR_LD_REPEAT_MISPREDICTION,
  CORR_LD_BITLENGTH_CORRECTION,
  CORR_TC_COUNT_MISPREDICTION,
  CORR_TC_BITLENGTH_CORRECTION,
};

enum Codec { ENCODER, DECODER };

struct PreflateStatisticalModel;
struct PreflateCodecModel;

struct PreflateStatisticalEncoder {
  PreflateStatisticalEncoder(const PreflateStatisticalModel&);
  ~PreflateStatisticalEncoder();

  void encodeHeader();
  void encodeParameters(const PreflateParameters&);
  void encodeModel();
  void encode(const PreflateCorrectionType& type, const int value, const unsigned refvalue = 0);
  void encodeValue(const unsigned value, const unsigned maxBits);
  static void encodeValue(aricoder* codec, const unsigned value, const unsigned maxBits);
  std::vector<unsigned char> encodeFinish();

  iostream* data;
  aricoder* codec;
  PreflateCodecModel* model;
};

struct PreflateStatisticalDecoder {
  PreflateStatisticalDecoder(const std::vector<unsigned char>&);
  ~PreflateStatisticalDecoder();

  bool decodeHeader();
  bool decodeParameters(PreflateParameters&);
  bool decodeModel();
  int decode(const PreflateCorrectionType& type, const unsigned refvalue = 0);
  unsigned decodeValue(const unsigned maxBits);
  void decodeFinish();
  static unsigned decodeValue(aricoder*, const unsigned maxBits);

  std::vector<unsigned char> storage;
  iostream* data;
  aricoder* codec;
  PreflateCodecModel* model;
};

bool isEqual(const PreflateCodecModel&, const PreflateCodecModel&);

#endif /* PREFLATE_STATISTICAL_CODEC_H */
