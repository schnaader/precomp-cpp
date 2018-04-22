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

#ifndef PREFLATE_STATISTICS_COUNTER_H
#define PREFLATE_STATISTICS_COUNTER_H

#include <algorithm>

struct PreflateStatisticsCounter {
  struct BlockPrediction {
  public:
    void incBlockType(const unsigned bt) {
      blockType[bt]++;
    }
    void incEOBPredictionWrong(const bool mispredicted) {
      EOBMisprediction[mispredicted]++;
    }
    void incNonZeroPadding(const bool nonzeropadding) {
      nonZeroPadding[nonzeropadding]++;
    }

    static unsigned totalModels() {
      return 3;
    }
    unsigned checkDefaultModels() const;

    void print();

  private:
    unsigned blockType[3]; // stored, dynamic huff, static huff
    unsigned EOBMisprediction[2]; // no, yes
    unsigned nonZeroPadding[2]; // no, yes

    friend struct PreflateBlockPredictionModel;
  };

  struct TreeCodePrediction {
  public:
    void incTreeCodeCountPredictionWrong(const bool mispredicted) {
      TCCountMisprediction[mispredicted]++;
    }
    void incTreeCodeLengthDiffToPrediction(const int len_diff) {
      TCBitlengthCorrection[std::max(std::min(len_diff, 3), -3) + 3]++;
    }
    void incLiteralCountPredictionWrong(const bool mispredicted) {
      LCountMisprediction[mispredicted]++;
    }
    void incDistanceCountPredictionWrong(const bool mispredicted) {
      DCountMisprediction[mispredicted]++;
    }
    void incLDCodeTypePredictionWrong(const unsigned codetype, const bool mispredicted) {
      LDTypeMisprediction[codetype][mispredicted]++;
    }
    void incLDCodeTypeReplacement(const unsigned replacement_codetype) {
      LDTypeReplacement[replacement_codetype]++;
    }
    void incLDCodeRepeatDiffToPrediction(const int len_diff) {
      LDRepeatCountCorrection[std::max(std::min(len_diff, 1), -1) + 1]++;
    }
    void incLDCodeLengthDiffToPrediction(const int len_diff) {
      LDBitlengthCorrection[std::max(std::min(len_diff, 4), -4) + 4]++;
    }

    static unsigned totalModels() {
      return 11;
    }
    unsigned checkDefaultModels() const;

    void print();

  private:
    unsigned TCCountMisprediction[2]; // no, yes
    unsigned TCBitlengthCorrection[7]; // -x, -2, -1, 0, +1, +2, +x
    unsigned LCountMisprediction[2]; // no, yes
    unsigned DCountMisprediction[2]; // no, yes
    unsigned LDTypeMisprediction[4][2]; // types: BL,REP,REPZS,REPZL; no, yes
    unsigned LDTypeReplacement[4];      // replacement type: BL,REP,REPZS,REPZL
    unsigned LDRepeatCountCorrection[3]; // -x, 0, +x
    unsigned LDBitlengthCorrection[9]; // -x, -3, -2, -1, 0, +1, +2, +3, +x

    friend struct PreflateTreeCodePredictionModel;
  };
  struct TokenPrediction {
  public:
    void incLiteralPredictionWrong(const bool mispredicted) {
      LITMisprediction[mispredicted]++;
    }
    void incReferencePredictionWrong(const bool mispredicted) {
      REFMisprediction[mispredicted]++;
    }
    void incLengthDiffToPrediction(const int len_diff) {
      LENCorrection[std::max(std::min(len_diff, 6), -6) + 6]++;
    }
    void incIrregularLength258Encoding(const bool irregular) {
      LEN258IrregularEncoding[irregular]++;
    }
    void incDistanceDiffToPredictionAfterIncorrectLengthPrediction(const int len_diff) {
      DISTAfterLenCorrection[std::min(len_diff, 3)]++;
    }
    void incDistanceDiffToPredictionAfterCorrectLengthPrediction(const int len_diff) {
      DISTOnlyCorrection[std::min(len_diff, 3)]++;
    }

    static unsigned totalModels() {
      return 6;
    }
    unsigned checkDefaultModels() const;

    void print();

  private:
    unsigned LITMisprediction[2]; // no, yes
    unsigned REFMisprediction[2]; // no, yes
    unsigned LENCorrection[13];   // -x, -5, -4, -3, -2, -1, 0, +1, +2, +3, +4, +5, +x (bytes)
    unsigned LEN258IrregularEncoding[2]; // no, yes
    unsigned DISTAfterLenCorrection[4]; // +0, +1, +2, +x (hops)
    unsigned DISTOnlyCorrection[4]; // +0, +1, +2, +x (hops)

    friend struct PreflateTokenPredictionModel;
  };

public:
  PreflateStatisticsCounter() {}

  BlockPrediction block;
  TreeCodePrediction treecode;
  TokenPrediction token;

  void print();
};

#endif /* PREFLATE_STATISTICS_COUNTER_H */
