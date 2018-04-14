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
#include "preflate_constants.h"
#include "preflate_statistical_codec.h"
#include "preflate_statistical_model.h"
#include "preflate_tree_predictor.h"

PreflateTreePredictor::PreflateTreePredictor(
    const std::vector<unsigned char>& dump)
  : input(dump)
  , curPos(0)
  , predictionFailure(false) {
}

struct FreqIdxPair {
  unsigned freq;
  unsigned idx;
};
struct TreeNode {
  unsigned parent;
  unsigned idx;
};

/* ===========================================================================
* Compares to subtrees, using the tree depth as tie breaker when
* the subtrees have equal frequency. This minimizes the worst case length.
*/
bool pq_smaller(const FreqIdxPair& p1, const FreqIdxPair& p2, const unsigned char* nodeDepth) {
  return p1.freq < p2.freq || (p1.freq == p2.freq && nodeDepth[p1.idx] <= nodeDepth[p2.idx]);
}

/* ===========================================================================
* Restore the heap property by moving down the tree starting at node k,
* exchanging a node with the smallest of its two sons if necessary, stopping
* when the heap property is re-established (each father smaller than its
* two sons).
*/
void pq_downheap(FreqIdxPair* ptr, const unsigned index, const unsigned len, const unsigned char* depth) {
  unsigned k = index;
  FreqIdxPair v = ptr[k];
  unsigned j = k * 2 + 1;  /* left son of k */
  while (j < len) {
    /* Set j to the smallest of the two sons: */
    if (j + 1 < len && pq_smaller(ptr[j + 1], ptr[j], depth)) {
      j++;
    }
    /* Exit if v is smaller than both sons */
    if (pq_smaller(v, ptr[j], depth)) break;

    /* Exchange v with the smallest son */
    ptr[k] = ptr[j];
    k = j;

    /* And continue down the tree, setting j to the left son of k */
    j = k * 2 + 1;
  }
  ptr[k] = v;
}

void pq_makeheap(FreqIdxPair* ptr, const unsigned len, const unsigned char* depth) {
  for (unsigned n = (len - 1) / 2 + 1; n > 0; n--) {
    pq_downheap(ptr, n - 1, len, depth);
  }
}

FreqIdxPair pq_remove(FreqIdxPair* ptr, unsigned& len, const unsigned char* depth) {
  FreqIdxPair result = ptr[0];
  ptr[0] = ptr[--len];
  pq_downheap(ptr, 0, len, depth);
  return result;
}

unsigned PreflateTreePredictor::calcBitLengths(
    unsigned char* symBitLen,
    const unsigned* symFreq,
    const unsigned symCount,
    const unsigned maxBits,
    const unsigned minMaxCode) {
  FreqIdxPair toSort[PreflateConstants::L_CODES];
  TreeNode nodes[PreflateConstants::L_CODES * 2 + 1];
  unsigned char nodeBitLen[PreflateConstants::L_CODES * 2 + 1];
  unsigned char nodeDepth[PreflateConstants::L_CODES * 2 + 1];
  memset(nodeBitLen, 0, sizeof(nodeBitLen));
  memset(nodeDepth, 0, sizeof(nodeDepth));
  unsigned maxCode = 0, len = 0, nodeCount = 0, nodeId = symCount;
  for (unsigned i = 0; i < symCount; ++i) {
    if (symFreq[i]) {
      toSort[len++] = FreqIdxPair {symFreq[i], maxCode = i};
    }
  }
  if (len < 2) {
    memset(symBitLen, 0, symCount);
    symBitLen[maxCode] = 1;
    symBitLen[maxCode < 2 ? ++maxCode : 0] = 1;
    return std::max(minMaxCode, maxCode + 1);
  }

  pq_makeheap(toSort, len, nodeDepth);
  while (len > 1) {
    FreqIdxPair least1 = pq_remove(toSort, len, nodeDepth);
    FreqIdxPair least2 = toSort[0];
    toSort[0] = FreqIdxPair {least1.freq + least2.freq, nodeId};
    nodes[nodeCount++] = TreeNode {nodeId, least1.idx};
    nodes[nodeCount++] = TreeNode {nodeId, least2.idx};
    nodeDepth[nodeId] = std::max(nodeDepth[least1.idx], nodeDepth[least2.idx]) + 1;
    // note? original code put new entry at top of heap, and moved it downwards
    // while push_heap pushes it upwards
    pq_downheap(toSort, 0, len, nodeDepth);
    nodeId++;
  }
  unsigned overflow = 0;
  unsigned bl_count[16];
  memset(bl_count, 0, sizeof(bl_count));
  unsigned orgNodeCount = nodeCount;
  while (nodeCount-- > 0) {
    unsigned char newLen = nodeBitLen[nodes[nodeCount].parent] + 1;
    if (newLen > maxBits) {
      newLen = maxBits;
      ++overflow;
    }
    unsigned idx = nodes[nodeCount].idx;
    nodeBitLen[idx] = newLen;
    if (idx < symCount) {
      bl_count[newLen]++;
    }
  }

  if (overflow) {
    unsigned bits;
    do {
      for (bits = maxBits - 1; bl_count[bits] == 0; bits--) {
      }
      bl_count[bits]--;      /* move one leaf down the tree */
      bl_count[bits + 1] += 2; /* move one overflow item as its brother */
      bl_count[maxBits]--;
      /* The brother of the overflow item also moves one step up,
      * but this does not affect bl_count[max_length]
      */
      overflow -= 2;
    } while (overflow > 0);

    for (bits = maxBits, nodeCount = orgNodeCount; nodeCount > 0; ) {
      --nodeCount;
      unsigned idx = nodes[nodeCount].idx;
      if (idx >= symCount) {
        continue;
      }
      while (bl_count[bits] == 0) {
        bits--;
      }
      nodeBitLen[idx] = bits;
      bl_count[bits]--;
    }
  }
  memcpy(symBitLen, nodeBitLen, symCount);
  return std::max(minMaxCode, maxCode + 1);
}

TreeCodeType PreflateTreePredictor::predictCodeType(const unsigned char* symBitLen,
                                                   const unsigned symCount,
                                                   const bool first) {
  unsigned char code = symBitLen[0];
  if (code == 0) {
    unsigned char curlen = 1;
    unsigned char maxCurLen = std::min(symCount, 11u);
    while (curlen < maxCurLen && symBitLen[curlen] == 0) {
      ++curlen;
    }
    if (curlen >= 11) {
      return TCT_REPZL;
    }
    if (curlen >= 3) {
      return TCT_REPZS;
    }
    return TCT_BITS;
  }
  if (!first && code == symBitLen[-1]) {
    unsigned char curlen = 1;
    unsigned char maxCurLen = std::min(symCount, 3u);
    while (curlen < maxCurLen && symBitLen[curlen] == code) {
      ++curlen;
    }
    if (curlen >= 3) {
      return TCT_REP;
    }
  }
  return TCT_BITS;
}
unsigned char PreflateTreePredictor::predictCodeData(const unsigned char* symBitLen,
                                                    const TreeCodeType type,
                                                    const unsigned symCount,
                                                    const bool first) {
  unsigned char code = symBitLen[0];
  switch (type) {
  default:
  case TCT_BITS:
    return code;
  case TCT_REP:
  {
    unsigned char curlen = 3;
    unsigned char maxCurLen = std::min(symCount, 6u);
    while (curlen < maxCurLen && symBitLen[curlen] == code) {
      ++curlen;
    }
    return curlen;
  }
  case TCT_REPZS:
  case TCT_REPZL:
  {
    unsigned char curlen = type == TCT_REPZS ? 3 : 11;
    unsigned char maxCurLen = std::min(symCount, type == TCT_REPZS ? 10u : 138u);
    while (curlen < maxCurLen && symBitLen[curlen] == 0) {
      ++curlen;
    }
    return curlen;
  }
  }
}


void PreflateTreePredictor::predictLDTrees(
    BlockAnalysisResult& analysis,
    unsigned* frequencies,
    const unsigned char* symBitLen,
    const unsigned symLCount,
    const unsigned symDCount,
    const unsigned char* targetCodes,
    const unsigned targetCodeSize) {
  memset(frequencies, 0, sizeof(unsigned) * PreflateConstants::BL_CODES);
  const unsigned char* ptr = symBitLen;
  const unsigned char* code = targetCodes;
  unsigned codeSize = targetCodeSize;
  unsigned count1 = symLCount;
  unsigned count2 = symDCount;
  bool first = true;
  while (codeSize > 0) {
    TreeCodeType targetTreeCodeType;
    switch (code[0]) {
    case 16: targetTreeCodeType = TCT_REP; break;
    case 17: targetTreeCodeType = TCT_REPZS; break;
    case 18: targetTreeCodeType = TCT_REPZL; break;
    default: targetTreeCodeType = TCT_BITS; break;
    }
    if (codeSize < 2 && targetTreeCodeType != TCT_BITS) {
      predictionFailure = true;
      return;
    }
    TreeCodeType predictedTreeCodeType = predictCodeType(ptr, count1, first);
    unsigned char info = predictedTreeCodeType | ((targetTreeCodeType != predictedTreeCodeType) << 2);
    if (targetTreeCodeType != predictedTreeCodeType) {
      analysis.correctives.push_back(targetTreeCodeType);
    }
    unsigned char targetTreeCodeData = code[targetTreeCodeType != TCT_BITS];
    unsigned l = 1 + (targetTreeCodeType != TCT_BITS);
    code += l;
    codeSize -= l;
    unsigned char predictedTreeCodeData = predictCodeData(ptr, targetTreeCodeType, count1, first);
    first = false;
    if (targetTreeCodeType != TCT_BITS) {
      if (targetTreeCodeData != predictedTreeCodeData) {
        info |= 8;
        analysis.correctives.push_back(targetTreeCodeData);
      }
    } else {
      analysis.correctives.push_back(predictedTreeCodeData);
      analysis.correctives.push_back(targetTreeCodeData - predictedTreeCodeData);
    }
    if (targetTreeCodeType != TCT_BITS) {
      frequencies[targetTreeCodeType + 15]++;
      l = targetTreeCodeData;
    } else {
      frequencies[targetTreeCodeData]++;
      l = 1;
    }
    ptr += l;
    if (count1 > l) {
      count1 -= l;
    } else {
      count1 += count2;
      count2 = 0;
      first = true;
      if (count1 >= l) {
        count1 -= l;
      } else {
        predictionFailure = true;
        return;
      }
    }
    analysis.tokenInfo.push_back(info);
  }
  analysis.tokenInfo.push_back(0xff);
  if (count1 + count2 != 0) {
    predictionFailure = true;
  }
}

void PreflateTreePredictor::collectTokenStatistics(
    unsigned Lcodes[],
    unsigned Dcodes[],
    unsigned& Lcount,
    unsigned& Dcount,
    const PreflateTokenBlock& block) {
  memset(Lcodes, 0, sizeof(unsigned) * PreflateConstants::L_CODES);
  memset(Dcodes, 0, sizeof(unsigned) * PreflateConstants::D_CODES);
  Lcount = 0;
  Dcount = 0;
  for (unsigned i = 0, n = block.tokens.size(); i < n; ++i) {
    PreflateToken targetToken = block.tokens[i];
    if (targetToken.len == 1) {
      Lcodes[input.curChar()]++;
      Lcount++;
      input.advance(1);
    } else {
      Lcodes[PreflateConstants::LITERALS + 1 + PreflateConstants::LCode(targetToken.len)]++;
      Lcount++;
      Dcodes[PreflateConstants::DCode(targetToken.dist)]++;
      Dcount++;
      input.advance(targetToken.len);
    }
  }
  Lcodes[256] = 1;
}
unsigned PreflateTreePredictor::buildLBitlenghs(
    unsigned char bitLengths[],
    unsigned Lcodes[]) {
  return calcBitLengths(bitLengths, Lcodes, PreflateConstants::L_CODES, 15, PreflateConstants::LITERALS + 1);
}
unsigned PreflateTreePredictor::buildDBitlenghs(
  unsigned char bitLengths[],
  unsigned Dcodes[]) {
  return calcBitLengths(bitLengths, Dcodes, PreflateConstants::D_CODES, 15, 0);
}
unsigned PreflateTreePredictor::buildTCBitlengths(
    unsigned char (&simpleCodeTree)[PreflateConstants::BL_CODES],
    unsigned (&BLfreqs)[PreflateConstants::BL_CODES]) {
  memset(simpleCodeTree, 0, sizeof(simpleCodeTree));
  calcBitLengths(simpleCodeTree, BLfreqs, PreflateConstants::BL_CODES, 7, 0);
  unsigned predictedCTreeSize = PreflateConstants::BL_CODES;
  while (predictedCTreeSize > 4 
         && simpleCodeTree[PreflateConstants::treeCodeOrderTable[predictedCTreeSize - 1]] == 0) {
    --predictedCTreeSize;
  }
  return predictedCTreeSize;
}

void PreflateTreePredictor::analyzeBlock(
    const unsigned blockno,
    const PreflateTokenBlock& block) {
  if (blockno != analysisResults.size() || predictionFailure) {
    return;
  }
  analysisResults.push_back(BlockAnalysisResult());
  BlockAnalysisResult& analysis = analysisResults[blockno];
  analysis.blockType = block.type;
  if (analysis.blockType != PreflateTokenBlock::DYNAMIC_HUFF) {
    return;
  }

  unsigned Lcodes[PreflateConstants::L_CODES], Dcodes[PreflateConstants::D_CODES];
  unsigned Lcount = 0, Dcount = 0;
  collectTokenStatistics(Lcodes, Dcodes, Lcount, Dcount, block);

  unsigned char bitLengths[PreflateConstants::LD_CODES];
  memset(bitLengths, 0, sizeof(bitLengths));
  unsigned predictedLTreeSize = buildLBitlenghs(bitLengths, Lcodes);
  analysis.tokenInfo.push_back(predictedLTreeSize != block.nlen);
  if (predictedLTreeSize != block.nlen) {
    analysis.correctives.push_back(block.nlen);
  }
  predictedLTreeSize = block.nlen;

  unsigned predictedDTreeSize = buildDBitlenghs(bitLengths + predictedLTreeSize, Dcodes);
  analysis.tokenInfo.push_back(predictedDTreeSize != block.ndist);
  if (predictedDTreeSize != block.ndist) {
    analysis.correctives.push_back(block.ndist);
  }
  predictedDTreeSize = block.ndist;

  unsigned BLfreqs[PreflateConstants::BL_CODES];
  const unsigned char* targetCodes = &block.treecodes[0];
  unsigned targetCodeSize = block.treecodes.size();
  predictLDTrees(analysis, BLfreqs, bitLengths, predictedLTreeSize, predictedDTreeSize, targetCodes + block.ncode, targetCodeSize - block.ncode);

  unsigned char simpleCodeTree[PreflateConstants::BL_CODES];
  unsigned predictedCTreeSize = buildTCBitlengths(simpleCodeTree, BLfreqs);
  analysis.tokenInfo.push_back(block.ncode);
  analysis.tokenInfo.push_back(predictedCTreeSize != block.ncode);
  predictedCTreeSize = block.ncode;
  for (unsigned i = 0; i < predictedCTreeSize; ++i) {
    unsigned predictedBL = simpleCodeTree[PreflateConstants::treeCodeOrderTable[i]];
    analysis.correctives.push_back(predictedBL);
    analysis.correctives.push_back(targetCodes[i] - predictedBL);
  }
}
void PreflateTreePredictor::encodeBlock(
  PreflateStatisticalEncoder* codec,
  const unsigned blockno) {
  BlockAnalysisResult& analysis = analysisResults[blockno];
  if (analysis.blockType != PreflateTokenBlock::DYNAMIC_HUFF) {
    return;
  }

  unsigned infoPos = 0, correctivePos = 0;
  unsigned char info = analysis.tokenInfo[infoPos++];
  codec->encode(CORR_L_COUNT_MISPREDICTION, info);
  if (info) {
    codec->encodeValue(analysis.correctives[correctivePos++] - PreflateConstants::LITERALS - 1, 5);
  }
  info = analysis.tokenInfo[infoPos++];
  codec->encode(CORR_D_COUNT_MISPREDICTION, info);
  if (info) {
    codec->encodeValue(analysis.correctives[correctivePos++], 5);
  }

  while ((info = analysis.tokenInfo[infoPos++]) != 0xff) {
    unsigned type = (info & 3);
    codec->encode(CORR_LD_TYPE_MISPREDICTION, (info & 4) != 0, type);
    if (info & 4) {
      unsigned newType = analysis.correctives[correctivePos++];
      codec->encode(CORR_LD_TYPE_REPLACEMENT, newType, type);
      type = newType;
    }
    if (type != TCT_BITS) {
      codec->encode(CORR_LD_REPEAT_MISPREDICTION, (info & 8) != 0);
      if (info & 8) {
        unsigned newRepeat = analysis.correctives[correctivePos++];
        switch (type) {
        case TCT_REP:
          codec->encodeValue(newRepeat - 3, 2);
          break;
        case TCT_REPZS:
          codec->encodeValue(newRepeat - 3, 3);
          break;
        case TCT_REPZL:
          codec->encodeValue(newRepeat - 11, 7);
          break;
        }
      }
    } else {
      unsigned bl_pred = analysis.correctives[correctivePos++];
      int bl_diff = analysis.correctives[correctivePos++];
      codec->encode(CORR_LD_BITLENGTH_CORRECTION, bl_diff, bl_pred);
    }
  }
  unsigned blcount = analysis.tokenInfo[infoPos++];
  info = analysis.tokenInfo[infoPos++];
  codec->encode(CORR_TC_COUNT_MISPREDICTION, info);
  if (info) {
    codec->encodeValue(blcount - 4, 4);
  }
  for (unsigned i = 0; i < blcount; ++i) {
    int bl_pred = analysis.correctives[correctivePos++];
    int bl_diff = analysis.correctives[correctivePos++];
    codec->encode(CORR_TC_BITLENGTH_CORRECTION, bl_diff, bl_pred);
  }
}

void PreflateTreePredictor::updateModel(
  PreflateStatisticalModel* model,
  const unsigned blockno) {
  BlockAnalysisResult& analysis = analysisResults[blockno];
  if (analysis.blockType != PreflateTokenBlock::DYNAMIC_HUFF) {
    return;
  }

  unsigned infoPos = 0, correctivePos = 0;
  unsigned char info = analysis.tokenInfo[infoPos++];
  model->LCountMisprediction[info]++;
  if (info) {
    correctivePos++;
  }
  info = analysis.tokenInfo[infoPos++];
  model->DCountMisprediction[info]++;
  if (info) {
    correctivePos++;
  }

  while ((info = analysis.tokenInfo[infoPos++]) != 0xff) {
    unsigned type = (info & 3);
    model->LDTypeMisprediction[type][(info & 4) != 0]++;
    if (info & 4) {
      unsigned newType = analysis.correctives[correctivePos++];
      model->LDTypeReplacement[newType]++;
      type = newType;
    }
    if (type != TCT_BITS) {
      model->LDRepeatCountMisprediction[(info & 8) != 0]++;
      if (info & 8) {
        correctivePos++;
      }
    } else {
      /*unsigned bl_pred = analysis.correctives[*/correctivePos++/*]*/;
      int bl_diff = analysis.correctives[correctivePos++];
      if (bl_diff >= 0) {
        model->LDBitlengthPositiveCorrection[std::min(4, bl_diff)]++;
      } else {
        model->LDBitlengthNegativeCorrection[std::min(3, -bl_diff - 1)]++;
      }
    }
  }
  unsigned blcount = analysis.tokenInfo[infoPos++];
  info = analysis.tokenInfo[infoPos++];
  model->TCCountMisprediction[info]++;
  for (unsigned i = 0; i < blcount; ++i) {
    /*int bl_pred = analysis.correctives[*/correctivePos++/*]*/;
    int bl_diff = analysis.correctives[correctivePos++];
    if (bl_diff >= 0) {
      model->TCBitlengthPositiveCorrection[std::min(3, bl_diff)]++;
    } else {
      model->TCBitlengthNegativeCorrection[std::min(2, -bl_diff - 1)]++;
    }
  }
}


unsigned PreflateTreePredictor::reconstructLDTrees(
    PreflateStatisticalDecoder* codec,
    unsigned* frequencies,
    unsigned char* targetCodes,
    const unsigned targetCodeSize,
    const unsigned char* symBitLen,
    const unsigned symLCount,
    const unsigned symDCount) {
  memset(frequencies, 0, sizeof(unsigned) * PreflateConstants::BL_CODES);
  const unsigned char* ptr = symBitLen;
  unsigned osize = 0;
  unsigned count1 = symLCount;
  unsigned count2 = symDCount;
  bool first = true;
  while (count1 + count2 > 0) {
    TreeCodeType predictedTreeCodeType = predictCodeType(ptr, count1, first);
    if (codec->decode(CORR_LD_TYPE_MISPREDICTION, predictedTreeCodeType)) {
      unsigned newType = codec->decode(CORR_LD_TYPE_REPLACEMENT, predictedTreeCodeType);
      switch (newType) {
      case TCT_BITS:
        predictedTreeCodeType = TCT_BITS;
        break;
      case TCT_REP:
        predictedTreeCodeType = TCT_REP;
        break;
      case TCT_REPZS:
        predictedTreeCodeType = TCT_REPZS;
        break;
      case TCT_REPZL:
        predictedTreeCodeType = TCT_REPZL;
        break;
      }
    }
    unsigned char predictedTreeCodeData = predictCodeData(ptr, predictedTreeCodeType, count1, first);
    first = false;
    if (predictedTreeCodeType != TCT_BITS) {
      if (codec->decode(CORR_LD_REPEAT_MISPREDICTION)) {
        switch (predictedTreeCodeType) {
        case TCT_REP:
          predictedTreeCodeData = codec->decodeValue(2) + 3;
          break;
        case TCT_REPZS:
          predictedTreeCodeData = codec->decodeValue(3) + 3;
          break;
        case TCT_REPZL:
          predictedTreeCodeData = codec->decodeValue(7) + 11;
          break;
        case TCT_BITS:
          // unreachable
          break;
        }
      }
    } else {
      int bl_diff = codec->decode(CORR_LD_BITLENGTH_CORRECTION, predictedTreeCodeData);
      predictedTreeCodeData += bl_diff;
    }
    unsigned l;
    if (predictedTreeCodeType != TCT_BITS) {
      frequencies[predictedTreeCodeType + 15]++;
      l = predictedTreeCodeData;
      if (osize + 2 > targetCodeSize) {
        predictionFailure = true;
        break;
      }
      targetCodes[osize++] = predictedTreeCodeType + 15;
      targetCodes[osize++] = predictedTreeCodeData;
    } else {
      frequencies[predictedTreeCodeData]++;
      l = 1;
      if (osize >= targetCodeSize) {
        predictionFailure = true;
        break;
      }
      targetCodes[osize++] = predictedTreeCodeData;
    }
    ptr += l;
    if (count1 > l) {
      count1 -= l;
    } else {
      count1 += count2;
      count2 = 0;
      first = true;
      if (count1 >= l) {
        count1 -= l;
      } else {
        predictionFailure = true;
        break;
      }
    }
  }
  if (count1 + count2 != 0) {
    predictionFailure = true;
  }
  return predictionFailure ? 0 : osize;
}

bool PreflateTreePredictor::decodeBlock(
    PreflateTokenBlock& block, 
    PreflateStatisticalDecoder* codec) {
  if (block.type != PreflateTokenBlock::DYNAMIC_HUFF) {
    return true;
  }

  unsigned Lcodes[PreflateConstants::L_CODES], Dcodes[PreflateConstants::D_CODES];
  unsigned Lcount = 0, Dcount = 0;
  collectTokenStatistics(Lcodes, Dcodes, Lcount, Dcount, block);

  unsigned char bitLengths[PreflateConstants::LD_CODES];
  memset(bitLengths, 0, sizeof(bitLengths));
  unsigned predictedLTreeSize = buildLBitlenghs(bitLengths, Lcodes);
  if (codec->decode(CORR_L_COUNT_MISPREDICTION)) {
    predictedLTreeSize = codec->decodeValue(5) + PreflateConstants::LITERALS + 1;
  }
  block.nlen = predictedLTreeSize;

  unsigned predictedDTreeSize = buildDBitlenghs(bitLengths + predictedLTreeSize, Dcodes);
  if (codec->decode(CORR_D_COUNT_MISPREDICTION)) {
    predictedDTreeSize = codec->decodeValue(5);
  }
  block.ndist = predictedDTreeSize;

  unsigned BLfreqs[PreflateConstants::BL_CODES];
  unsigned char compressedLDtrees[PreflateConstants::LD_CODES];
  unsigned targetCodeSize = reconstructLDTrees(codec, BLfreqs, compressedLDtrees, PreflateConstants::LD_CODES,
                                               bitLengths, predictedLTreeSize, predictedDTreeSize);
  if (predictionFailure) {
    return false;
  }

  unsigned char simpleCodeTree[PreflateConstants::BL_CODES];
  unsigned predictedCTreeSize = buildTCBitlengths(simpleCodeTree, BLfreqs);
  if (codec->decode(CORR_TC_COUNT_MISPREDICTION)) {
    predictedCTreeSize = codec->decodeValue(4) + 4;
  }
  block.ncode = predictedCTreeSize;
  unsigned char shuffledCodeTree[PreflateConstants::BL_CODES];
  for (unsigned i = 0; i < predictedCTreeSize; ++i) {
    unsigned predictedBL = simpleCodeTree[PreflateConstants::treeCodeOrderTable[i]];
    int bl_diff = codec->decode(CORR_TC_BITLENGTH_CORRECTION, predictedBL);
    shuffledCodeTree[i] = predictedBL + bl_diff;
  }
  block.treecodes.reserve(predictedCTreeSize + targetCodeSize);
  block.treecodes.insert(block.treecodes.end(), shuffledCodeTree, shuffledCodeTree + predictedCTreeSize);
  block.treecodes.insert(block.treecodes.end(), compressedLDtrees, compressedLDtrees + targetCodeSize);
  return true;
}
