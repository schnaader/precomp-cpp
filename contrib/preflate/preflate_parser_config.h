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

#ifndef PREFLATE_PARSER_CONFIG
#define PREFLATE_PARSER_CONFIG

/* Values for max_lazy_match, good_match and max_chain_length, depending on
* the desired pack level (0..9). The values given below have been tuned to
* exclude worst case performance for pathological files. Better values may be
* found for specific files.
*/
struct PreflateParserConfig {
  unsigned char good_length; /* reduce lazy search above this match length */
  unsigned short max_lazy;    /* do not perform lazy search above this match length */
  unsigned short nice_length; /* quit search above this match length */
  unsigned short max_chain;
};

extern const PreflateParserConfig fastPreflateParserSettings[3];
extern const PreflateParserConfig slowPreflateParserSettings[6];


#endif 
/* PREFLATE_PARSER_CONFIG */