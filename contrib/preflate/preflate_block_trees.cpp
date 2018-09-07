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
#include "preflate_block_trees.h"
#include "support/bit_helper.h"

static HuffmanDecoder* staticLitLenDecoder;
static HuffmanDecoder* staticDistDecoder;
static HuffmanEncoder* staticLitLenEncoder;
static HuffmanEncoder* staticDistEncoder;

static void setLitLenBitLengths(unsigned char(&a)[288]) {
  std::fill(a +   0, a + 144, 8);
  std::fill(a + 144, a + 256, 9);
  std::fill(a + 256, a + 280, 7);
  std::fill(a + 280, a + 288, 8);
}
static void setDistBitLengths(unsigned char(&a)[32]) {
  std::fill(a, a + 32, 5);
}

const HuffmanDecoder* PreflateBlockTrees::staticLitLenTreeDecoder() {
  if (!staticLitLenDecoder) {
    unsigned char l_lengths[288];
    setLitLenBitLengths(l_lengths);
    staticLitLenDecoder = new HuffmanDecoder(l_lengths, 288, true, 15);
  }
  return staticLitLenDecoder;
}
const HuffmanDecoder* PreflateBlockTrees::staticDistTreeDecoder() {
  if (!staticDistDecoder) {
    unsigned char d_lengths[32];
    setDistBitLengths(d_lengths);
    staticDistDecoder = new HuffmanDecoder(d_lengths, 32, true, 15);
  }
  return staticDistDecoder;
}
const HuffmanEncoder* PreflateBlockTrees::staticLitLenTreeEncoder() {
  if (!staticLitLenEncoder) {
    unsigned char l_lengths[288];
    setLitLenBitLengths(l_lengths);
    staticLitLenEncoder = new HuffmanEncoder(l_lengths, 288, true);
  }
  return staticLitLenEncoder;
}
const HuffmanEncoder* PreflateBlockTrees::staticDistTreeEncoder() {
  if (!staticDistEncoder) {
    unsigned char d_lengths[32];
    setDistBitLengths(d_lengths);
    staticDistEncoder = new HuffmanEncoder(d_lengths, 32, true);
  }
  return staticDistEncoder;
}
