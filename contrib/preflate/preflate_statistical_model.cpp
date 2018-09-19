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

#include "preflate_statistical_model.h"
#include "preflate_token.h"
#include "support/array_helper.h"
#include <stdio.h>

unsigned PreflateStatisticsCounter::BlockPrediction::checkDefaultModels() const {
  unsigned cnt = 0;
  cnt += sumArray(blockType) == blockType[PreflateTokenBlock::DYNAMIC_HUFF];
  cnt += sumArray(EOBMisprediction) == EOBMisprediction[0];
  cnt += sumArray(nonZeroPadding) == nonZeroPadding[0];
  return cnt;
}

unsigned PreflateStatisticsCounter::TreeCodePrediction::checkDefaultModels() const {
  unsigned cnt = 0;
  cnt += sumArray(TCCountMisprediction) == TCCountMisprediction[0];
  cnt += sumArray(TCBitlengthCorrection) == TCBitlengthCorrection[3];
  cnt += sumArray(LCountMisprediction) == LCountMisprediction[0];
  cnt += sumArray(DCountMisprediction) == DCountMisprediction[0];
  for (unsigned i = 0; i < 4; ++i) {
    cnt += sumArray(LDTypeMisprediction[i]) == LDTypeMisprediction[i][0];
  }
  cnt += sumArray(LDTypeReplacement) == 0;
  cnt += sumArray(LDRepeatCountCorrection) == LDRepeatCountCorrection[1];
  cnt += sumArray(LDBitlengthCorrection) == LDBitlengthCorrection[4];
  return cnt;
}

unsigned PreflateStatisticsCounter::TokenPrediction::checkDefaultModels() const {
  unsigned cnt = 0;
  cnt += sumArray(LITMisprediction) == LITMisprediction[0];
  cnt += sumArray(REFMisprediction) == REFMisprediction[0];
  cnt += sumArray(LENCorrection) == LENCorrection[6];
  cnt += sumArray(DISTAfterLenCorrection) == DISTAfterLenCorrection[0];
  cnt += sumArray(DISTOnlyCorrection) == DISTOnlyCorrection[0];
  cnt += sumArray(LEN258IrregularEncoding) == LEN258IrregularEncoding[0];
  return cnt;
}
