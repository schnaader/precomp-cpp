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
#include "support/array_helper.h"
#include <stdio.h>

void printFlagStatistics(const char *txt, unsigned(&flag)[2]) {
  if (flag[1]) {
    printf("%s %g%% (%d)", txt, flag[1] * 100.0 / (flag[0] + flag[1]), flag[0] + flag[1]);
  }
}
void printArrayStatistics(const char *txt, const char* txt2, unsigned data[], unsigned size, unsigned sum, int offset) {
  bool on = false;
  for (unsigned i = 0; i < size; ++i) {
    if (data[i]) {
      on = true;
      if (i + 1 == size && txt2) {
        printf("%s %g%%", txt2, data[i] * 100.0 / sum);
      } else {
        printf("%s%d %g%%", txt, i + offset, data[i] * 100.0 / sum);
      }
    }
  }
  if (on) {
    printf(" (%d)", sum);
  }
}

void PreflateStatisticalModel::print() {
  printFlagStatistics(", EOB MP", EOBMisprediction);
  printFlagStatistics(", !LIT MP", LITMisprediction);
  printFlagStatistics(", !REF MP", REFMisprediction);
  printFlagStatistics(", !LEN MP", LENMisprediction);
  printFlagStatistics(", !DIST MP", DISTOnlyMisprediction);
  unsigned sum;
  if (LENMisprediction) {
    sum = sumArray(LENPositiveCorrection)
      + sumArray(LENNegativeCorrection);
    printArrayStatistics(" L+", " L+?", LENPositiveCorrection, 6, sum, 1);
    printArrayStatistics(" L-", " L-?", LENNegativeCorrection, 6, sum, 1);
  }
  sum = sumArray(DISTAfterLenCorrection);
  if (sum) {
    printArrayStatistics(" L->D+", " L->D+?", DISTAfterLenCorrection, 4, sum, 0);
  }
  sum = sumArray(DISTOnlyCorrection);
  if (sum) {
    printArrayStatistics(" ->D+", " L->D+?", DISTOnlyCorrection, 4, sum, 0);
  }

  printFlagStatistics(", !CT SZ MP", TCCountMisprediction);
  printFlagStatistics(", !L SZ MP", LCountMisprediction);
  printFlagStatistics(", !D SZ MP", DCountMisprediction);
  printFlagStatistics(", !T B MP", LDTypeMisprediction[0]);
  printFlagStatistics(", !T R MP", LDTypeMisprediction[1]);
  printFlagStatistics(", !T 0s MP", LDTypeMisprediction[2]);
  printFlagStatistics(", !T 0l MP", LDTypeMisprediction[3]);
  sum = sumArray(LDTypeReplacement);
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
  sum = sumArray(TCBitlengthPositiveCorrection) + sumArray(TCBitlengthNegativeCorrection);
  printArrayStatistics(", C BL+", ", C BL+?", TCBitlengthPositiveCorrection, 4, sum, 0);
  printArrayStatistics(", C BL-", ", C BL-?", TCBitlengthNegativeCorrection, 3, sum, 1);
  printFlagStatistics(" LD R MP", LDRepeatCountMisprediction);
  sum = sumArray(LDBitlengthPositiveCorrection) + sumArray(LDBitlengthNegativeCorrection);
  printArrayStatistics(", LD BL+", ", LD BL+?", LDBitlengthPositiveCorrection, 5, sum, 0);
  printArrayStatistics(", LD BL-", ", LD BL-?", LDBitlengthNegativeCorrection, 4, sum, 1);
  printf("\n");
}