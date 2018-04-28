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
    LITERAL_COUNT = 256,
    NONLEN_CODE_COUNT = LITERAL_COUNT + 1, // EOB
    LEN_CODE_COUNT = 29,
    LITLEN_CODE_COUNT = NONLEN_CODE_COUNT + LEN_CODE_COUNT,
    DIST_CODE_COUNT = 30,
    LITLENDIST_CODE_COUNT = LITLEN_CODE_COUNT + DIST_CODE_COUNT,
    CODETREE_CODE_COUNT = 19,

    MIN_MATCH = 3,
    MAX_MATCH = 258,

    MAX_BITS = 15,

    MIN_LOOKAHEAD = MAX_MATCH + MIN_MATCH + 1,
  };

  static const unsigned char distCodeTable[512];
  static const unsigned char lengthCodeTable[MAX_MATCH - MIN_MATCH + 1];
  static const unsigned char lengthBaseTable[LEN_CODE_COUNT];
  static const unsigned short distBaseTable[DIST_CODE_COUNT];

  static const unsigned char lengthExtraTable[LEN_CODE_COUNT];
  static const unsigned char distExtraTable[DIST_CODE_COUNT];
  static const unsigned char treeCodeOrderTable[CODETREE_CODE_COUNT];

  static inline unsigned DCode(const unsigned dist) {
    return distCodeTable[dist <= 256 ? dist - 1 : 256 + ((dist - 1) >> 7)];
  }
  static inline unsigned LCode(const unsigned len) {
    return lengthCodeTable[len - MIN_MATCH];
  }
};

#endif /* PREFLATE_CONSTANTS_H */
