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
#include "preflate_block_decoder.h"
#include "preflate_block_trees.h"
#include "support/bit_helper.h"

PreflateBlockDecoder::PreflateBlockDecoder(
    BitInputStream& input,
    OutputCacheStream& output) 
  : _input(input)
  , _output(output)
  , _errorCode(OK)
  , _dynamicLitLenDecoder(nullptr, 0, false, 0)
  , _dynamicDistDecoder(nullptr, 0, false, 0) {
}

bool PreflateBlockDecoder::_error(const ErrorCode code) {
  _errorCode = code;
  return false;
}

bool PreflateBlockDecoder::readBlock(PreflateTokenBlock &block, bool &last) {
  block.uncompressedStartPos = _output.cacheEndPos();
  int32_t earliest_reference = INT32_MAX, curPos = 0;

  if (_input.eof()) {
    return false;
  }

  last = _readBit() != 0;
  unsigned char mode = _readBits(2);
  switch (mode) {
  default:
    return false;
  case 0: {
    block.type = PreflateTokenBlock::STORED;
    block.paddingBitCount = (-_input.bitPos()) & 7;
    block.paddingBits = _input.get(block.paddingBitCount);
    size_t len = _readBits(16);
    size_t ilen = _readBits(16);
    if ((len ^ ilen) != 0xffff) {
      return _error(STORED_BLOCK_LEN_MISMATCH);
    }
    block.uncompressedLen = len;
    block.contextLen = 0;
    return _input.copyBytesTo(_output, len) == len;
  }
  case 1:
  case 2:
    if (mode == 1) {
      block.type = PreflateTokenBlock::STATIC_HUFF;
      _setupStaticTables();
    } else {
      block.type = PreflateTokenBlock::DYNAMIC_HUFF;
      if (!_readDynamicTables(block)) {
        return false;
      }
    }
    while (true) {
      if (_input.eof()) {
        return false;
      }
      unsigned litLen = _litLenDecoder->decode(_input);
      if (litLen < 256) {
        _writeLiteral(litLen);
        block.tokens.push_back(PreflateToken(PreflateToken::LITERAL));
        curPos++;
      } else if (litLen == 256) {
        block.uncompressedLen = _output.cacheEndPos() - block.uncompressedStartPos;
        block.contextLen = -earliest_reference;
        return true;
      } else if (litLen <= PreflateConstants::L_CODES) {
        unsigned lcode = litLen - PreflateConstants::LITERALS - 1;
        unsigned len = PreflateConstants::MIN_MATCH
          + PreflateConstants::lengthBaseTable[lcode]
          + _readBits(PreflateConstants::lengthExtraTable[lcode]);
        if (len == 258 && lcode != PreflateConstants::L_CODES - PreflateConstants::LITERALS - 2) {
          len |= 512;
        }
        unsigned dcode = _distDecoder->decode(_input);
        if (dcode > PreflateConstants::D_CODES) {
          return false;
        }
        unsigned dist = 1
          + PreflateConstants::distBaseTable[dcode]
          + _readBits(PreflateConstants::distExtraTable[dcode]);
        if (dist > _output.cacheEndPos()) {
          return false;
        }
        _writeReference(dist, len);
        block.tokens.push_back(PreflateToken(PreflateToken::REFERENCE, len, dist));
        earliest_reference = std::min(earliest_reference, curPos - (int32_t)dist);
        curPos += len;
      } else {
        return false;
      }
    }
  }
}

void PreflateBlockDecoder::_setupStaticTables() {
  _litLenDecoder = PreflateBlockTrees::staticLitLenTreeDecoder();
  _distDecoder = PreflateBlockTrees::staticDistTreeDecoder();
}

bool PreflateBlockDecoder::_readDynamicTables(PreflateTokenBlock& block) {
  block.nlen = PreflateConstants::LITERALS + 1 + _readBits(5);
  block.ndist = 1 + _readBits(5);
  block.ncode = 4 + _readBits(4);
  if (block.nlen > PreflateConstants::L_CODES || block.ndist > PreflateConstants::D_CODES) {
    return false;
  }
  block.treecodes.clear();
  block.treecodes.reserve(block.nlen + block.ndist + block.ncode);

  unsigned char tcBitLengths[PreflateConstants::BL_CODES];
  unsigned char ldBitLengths[PreflateConstants::LD_CODES];
  memset(tcBitLengths, 0, sizeof(tcBitLengths));
  memset(ldBitLengths, 0, sizeof(ldBitLengths));
  for (unsigned i = 0, n = block.ncode; i < n; ++i) {
    unsigned char tc = _readBits(3);
    block.treecodes.push_back(tc);
    tcBitLengths[PreflateConstants::treeCodeOrderTable[i]] = tc;
  }
  HuffmanDecoder tcTree(tcBitLengths, PreflateConstants::BL_CODES, true, 7);
  if (tcTree.error()) {
    return false;
  }
  for (unsigned i = 0, n = block.nlen + block.ndist; i < n; ++i) {
    unsigned char code = tcTree.decode(_input);
    if (code > 18) {
      return false;
    }
    block.treecodes.push_back(code);
    if (code < 16) {
      ldBitLengths[i] = code;
      continue;
    }
    unsigned char len = 0, tocopy = 0;
    switch (code) {
    case 16:
      if (i == 0) {
        return false;
      }
      tocopy = ldBitLengths[i - 1];
      len = 3 + _readBits(2);
      break;
    case 17:
      tocopy = 0;
      len = 3 + _readBits(3);
      break;
    case 18:
      tocopy = 0;
      len = 11 + _readBits(7);
      break;
    }
    if (i + len > n) {
      return false;
    }
    block.treecodes.push_back(len);
    memset(ldBitLengths + i, tocopy, len);
    i += len - 1;
  }
  if (!ldBitLengths[256]) {
    return false;
  }
  _dynamicLitLenDecoder = HuffmanDecoder(ldBitLengths, block.nlen, true, 15);
  if (_dynamicLitLenDecoder.error()) {
    return false;
  }
  _litLenDecoder = &_dynamicLitLenDecoder;

  _dynamicDistDecoder = HuffmanDecoder(ldBitLengths + block.nlen, block.ndist, true, 15);
  if (_dynamicDistDecoder.error()) {
    return false;
  }
  _distDecoder = &_dynamicDistDecoder;
  return true;
}
