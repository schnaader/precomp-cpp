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

#include "preflate_parser_config.h"

#include <algorithm>

// -----------------------------------------

/*      good lazy nice chain */
const PreflateParserConfig fastPreflateParserSettings[3] = {
  /* 1 */ {4,    4,   8,    4}, /* max speed, no lazy matches */
  /* 2 */ {4,    5,  16,    8},
  /* 3 */ {4,    6,  32,   32},
};
const PreflateParserConfig slowPreflateParserSettings[6] = {
  /* 4 */ {4,    4,  16,   16},  /* lazy matches */
  /* 5 */ {8,   16,  32,   32},
  /* 6 */ {8,   16, 128,  128},
  /* 7 */ {8,   32, 128,  256},
  /* 8 */ {32, 128, 258, 1024},
  /* 9 */ {32, 258, 258, 4096}, /* max compression */
};
