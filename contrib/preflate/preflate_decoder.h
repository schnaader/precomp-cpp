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

#ifndef PREFLATE_DECODER_H
#define PREFLATE_DECODER_H

#include <functional>
#include <vector>
#include "preflate_token.h"
#include "support/stream.h"

bool preflate_decode(std::vector<unsigned char>& unpacked_output,
                     std::vector<unsigned char>& preflate_diff,
                     const std::vector<unsigned char>& deflate_raw);

bool preflate_decode(std::vector<unsigned char>& unpacked_output,
                     std::vector<unsigned char>& preflate_diff,
                     uint64_t& deflate_size,
                     InputStream& deflate_raw,
                     std::function<void (void)> block_callback);

#endif /* PREFLATE_DECODER_H */
