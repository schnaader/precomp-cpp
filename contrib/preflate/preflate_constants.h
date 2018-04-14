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

#ifndef PREFLATE_CONSTANTS_H
#define PREFLATE_CONSTANTS_H

struct PreflateConstants {
  enum {
    LENGTH_CODES = 29,
    LITERALS = 256,
    L_CODES = LITERALS + 1 /* eob */ + LENGTH_CODES,
    D_CODES = 30,
    LD_CODES = L_CODES + D_CODES,
    BL_CODES = 19,

    MIN_MATCH = 3,
    MAX_MATCH = 258,

    MAX_BITS = 15,

    MIN_LOOKAHEAD = MAX_MATCH + MIN_MATCH + 1,
  };

  static const unsigned char distCodeTable[512];
  static const unsigned char lengthCodeTable[MAX_MATCH - MIN_MATCH + 1];
  static const unsigned char lengthBaseTable[LENGTH_CODES];
  static const unsigned short distBaseTable[D_CODES];

  static const unsigned char lengthExtraTable[LENGTH_CODES];
  static const unsigned char distExtraTable[D_CODES];
  static const unsigned char treeCodeOrderTable[BL_CODES];

  static inline unsigned DCode(const unsigned dist) {
    return distCodeTable[dist <= 256 ? dist - 1 : 256 + ((dist - 1) >> 7)];
  }
  static inline unsigned LCode(const unsigned len) {
    return lengthCodeTable[len - MIN_MATCH];
  }
};

#endif /* PREFLATE_CONSTANTS_H */
