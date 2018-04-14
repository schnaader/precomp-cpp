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

#ifndef PREFLATE_STATISTICAL_MODEL_H
#define PREFLATE_STATISTICAL_MODEL_H

//#include "packARI/source/bitops.h"
//#include "packARI/source/aricoder.h"

struct PreflateStatisticalModel {
  // Blocks
  unsigned blockType[3]; // stored, dynamic huff, static huff
  unsigned EOBMisprediction[2]; // no, yes
  // Tokens
  unsigned LITMisprediction[2]; // no, yes
  unsigned REFMisprediction[2]; // no, yes
  unsigned LENMisprediction[2]; // no, yes
  unsigned LENPositiveCorrection[6];   // +1, +2, +3, +4, +5, +x (bytes)
  unsigned LENNegativeCorrection[6];   // -1, -2, -3, -4, -5, -x (bytes)
  unsigned DISTAfterLenCorrection[4]; // +0, +1, +2, +x (hops)
  unsigned DISTOnlyMisprediction[2]; // no, yes
  unsigned DISTOnlyCorrection[4]; // +0, +1, +2, +x (hops)
  // Trees
  unsigned TCCountMisprediction[2]; // no, yes
  unsigned TCBitlengthPositiveCorrection[4]; // 0, +1, +2, +x
  unsigned TCBitlengthNegativeCorrection[3]; // -1, -2, -x
  unsigned LCountMisprediction[2]; // no, yes
  unsigned DCountMisprediction[2]; // no, yes
  unsigned LDTypeMisprediction[4][2]; // types: BL,REP,REPZS,REPZL; no, yes
  unsigned LDTypeReplacement[4];      // types: BL,REP,REPZS,REPZL; replacement type
  unsigned LDRepeatCountMisprediction[2]; // no, yes
  unsigned LDBitlengthPositiveCorrection[5]; // 0, +1, +2, +3, +x
  unsigned LDBitlengthNegativeCorrection[4]; // -1, -2, -3, -x, 

  void print();
};

//  iostream data;
//  aricoder ;

#endif /* PREFLATE_STATISTICAL_MODEL_H */
