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
#include "preflate_statistical_codec.h"
#include "preflate_statistical_model.h"
#include "support/array_helper.h"
#include "support/bit_helper.h"
#include <stdint.h>

template <unsigned N>
bool PreflateSubModel<N>::isEqualTo(const PreflateSubModel<N>& m) const {
  if (N == 0 || m.bounds[N] == 0) {
    return true;
  }
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

template <unsigned NEG, unsigned POS>
bool PreflateCorrectionSubModel<NEG, POS>::isEqualTo(const PreflateCorrectionSubModel<NEG, POS>& m) const {
  return sign.isEqualTo(m.sign)
    && pos.isEqualTo(m.pos)
    && neg.isEqualTo(m.neg);
}


bool PreflateBlockPredictionModel::isEqualTo(const PreflateBlockPredictionModel& m) const {
  return blockType.isEqualTo(m.blockType)
    && EOBMisprediction.isEqualTo(m.EOBMisprediction)
    && nonZeroPadding.isEqualTo(m.nonZeroPadding);
}
bool PreflateTreeCodePredictionModel::isEqualTo(const PreflateTreeCodePredictionModel& m) const {
  return TCBitlengthCorrection.isEqualTo(m.TCBitlengthCorrection)
    && TCCountMisprediction.isEqualTo(m.TCCountMisprediction)
    && LCountMisprediction.isEqualTo(m.LCountMisprediction)
    && DCountMisprediction.isEqualTo(m.DCountMisprediction)
    && LDTypeMisprediction[0].isEqualTo(m.LDTypeMisprediction[0])
    && LDTypeMisprediction[1].isEqualTo(m.LDTypeMisprediction[1])
    && LDTypeMisprediction[2].isEqualTo(m.LDTypeMisprediction[2])
    && LDTypeMisprediction[3].isEqualTo(m.LDTypeMisprediction[3])
    && LDTypeReplacementBase.isEqualTo(m.LDTypeReplacementBase)
    && LDRepeatCountCorrection.isEqualTo(m.LDRepeatCountCorrection)
    && LDBitlengthCorrection.isEqualTo(m.LDBitlengthCorrection);
}
bool PreflateTokenPredictionModel::isEqualTo(const PreflateTokenPredictionModel& m) const {
  return LITMisprediction.isEqualTo(m.LITMisprediction)
    && REFMisprediction.isEqualTo(m.REFMisprediction)
    && LENCorrection.isEqualTo(m.LENCorrection)
    && DISTAfterLenCorrection.isEqualTo(m.DISTAfterLenCorrection)
    && DISTOnlyCorrection.isEqualTo(m.DISTOnlyCorrection)
    && IrregularLen258Encoding.isEqualTo(m.IrregularLen258Encoding);
}

bool PreflatePredictionModel::isEqualTo(const PreflatePredictionModel& m) const {
  return block.isEqualTo(m.block)
    && treecode.isEqualTo(m.treecode)
    && token.isEqualTo(m.token);
}

bool isEqual(const PreflatePredictionModel& m1, const PreflatePredictionModel& m2) {
  return m1.isEqualTo(m2);
}

// ----------------------------

void printFlagStatistics(const char *txt, unsigned(&flag)[2]) {
  if (flag[1]) {
    printf("%s %g%% (%d)", txt, flag[1] * 100.0 / (flag[0] + flag[1]), flag[0] + flag[1]);
  }
}
void printCorrectionStatistics(const char *txt,
                               unsigned data[], unsigned size, unsigned sum, unsigned offset) {
  if (data[offset] == sum) {
    return;
  }
  bool on = false;
  for (unsigned i = 0; i < size; ++i) {
    if (data[i]) {
      if (!on) {
        printf("%s:", txt);
      }
      on = true;
      if (i != offset && (i == 0 || i + 1 == size)) {
        printf(" %sx %g%%", i == 0 ? "-" : "+", data[i] * 100.0 / sum);
      } else {
        printf(" %s%d %g%%", i == offset ? "" : (i < offset ? "-" : "+"), (int)labs((int)(i - offset)), data[i] * 100.0 / sum);
      }
    }
  }
  if (on) {
    printf(" (%d)", sum);
  }
}
template <unsigned N>
void printCorrectionStatistics(const char *txt, unsigned (&data)[N], unsigned sum, int offset) {
  printCorrectionStatistics(txt, data, N, sum, offset);
}

// ----------------------------

void PreflateStatisticsCounter::BlockPrediction::print() {
  unsigned sum = sumArray(blockType);
  if (blockType[0]) {
    printf(" ->STORE %g%%", blockType[0] * 100.0 / sum);
  }
  if (blockType[1] && blockType[1] != sum) {
    printf(" ->DYNHUF %g%%", blockType[1] * 100.0 / sum);
  }
  if (blockType[2]) {
    printf(" ->STATHUF %g%%", blockType[2] * 100.0 / sum);
  }
  printFlagStatistics(", EOB MP", EOBMisprediction);
  printFlagStatistics(", PAD!=0", nonZeroPadding);
}

void PreflateStatisticsCounter::TreeCodePrediction::print() {
  printFlagStatistics(", !CT SZ MP", TCCountMisprediction);
  printFlagStatistics(", !L SZ MP", LCountMisprediction);
  printFlagStatistics(", !D SZ MP", DCountMisprediction);
  printFlagStatistics(", !T B MP", LDTypeMisprediction[0]);
  printFlagStatistics(", !T R MP", LDTypeMisprediction[1]);
  printFlagStatistics(", !T 0s MP", LDTypeMisprediction[2]);
  printFlagStatistics(", !T 0l MP", LDTypeMisprediction[3]);
  unsigned sum = sumArray(LDTypeReplacement);
  if (LDTypeReplacement[0]) {
    printf(" ->T B %g", LDTypeReplacement[0] * 100.0 / sum);
  }
  if (LDTypeReplacement[1]) {
    printf(" ->T R %g", LDTypeReplacement[1] * 100.0 / sum);
  }
  if (LDTypeReplacement[2]) {
    printf(" ->T 0s %g", LDTypeReplacement[2] * 100.0 / sum);
  }
  if (LDTypeReplacement[3]) {
    printf(" ->T 0l %g", LDTypeReplacement[3] * 100.0 / sum);
  }
  sum = sumArray(TCBitlengthCorrection);
  printCorrectionStatistics(", C BL", TCBitlengthCorrection, sum, 3);
  sum = sumArray(LDRepeatCountCorrection);
  printCorrectionStatistics(" LD RP", LDRepeatCountCorrection, sum, 1);
  sum = sumArray(LDBitlengthCorrection);
  printCorrectionStatistics(", LD BL", LDBitlengthCorrection, sum, 4);
}

void PreflateStatisticsCounter::TokenPrediction::print() {
  printFlagStatistics(", !LIT MP", LITMisprediction);
  printFlagStatistics(", !REF MP", REFMisprediction);
  unsigned sum = sumArray(LENCorrection);
  printCorrectionStatistics(" L", LENCorrection, sum, 6);
  sum = sumArray(DISTAfterLenCorrection);
  printCorrectionStatistics(" L->D+", DISTAfterLenCorrection, sum, 0);
  sum = sumArray(DISTOnlyCorrection);
  printCorrectionStatistics(" ->D+", DISTOnlyCorrection, sum, 0);
  printFlagStatistics(", !L258 MP", LEN258IrregularEncoding);
}


void PreflateStatisticsCounter::print() {
  block.print();
  treecode.print();
  token.print();
  printf("\n");
}
