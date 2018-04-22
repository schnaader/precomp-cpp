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
#include "preflate_block_reencoder.h"
#include "preflate_block_trees.h"
#include "support/bit_helper.h"

PreflateBlockReencoder::PreflateBlockReencoder(
    BitOutputStream& bos,
    const std::vector<unsigned char>& uncompressedData)
  : _output(bos)
  , _uncompressedData(uncompressedData)
  , _uncompressedDataPos(0)
  , _errorCode(OK)
  , _dynamicLitLenEncoder(nullptr, 0, false)
  , _dynamicDistEncoder(nullptr, 0, false) {
}

bool PreflateBlockReencoder::_error(const ErrorCode code) {
  _errorCode = code;
  return false;
}

void PreflateBlockReencoder::_setupStaticTables() {
  _litLenEncoder = PreflateBlockTrees::staticLitLenTreeEncoder();
  _distEncoder   = PreflateBlockTrees::staticDistTreeEncoder();
}

bool PreflateBlockReencoder::_buildAndWriteDynamicTables(const PreflateTokenBlock& block) {
  if (block.ncode < 4 || block.ncode > PreflateConstants::BL_CODES
      || block.treecodes.size() < (size_t)block.ncode
      || block.nlen < PreflateConstants::LITERALS + 1
      || block.nlen > PreflateConstants::L_CODES
      || block.ndist < 1 || block.ndist > PreflateConstants::D_CODES) {
    return _error(TREE_OUT_OF_RANGE);
  }
  unsigned char tcBitLengths[PreflateConstants::BL_CODES];
  unsigned char ldBitLengths[PreflateConstants::LD_CODES];
  memset(tcBitLengths, 0, sizeof(tcBitLengths));
  memset(ldBitLengths, 0, sizeof(ldBitLengths));

  for (unsigned i = 0, n = block.ncode; i < n; ++i) {
    unsigned char tc = block.treecodes[i];
    _output.put(tc, 3);
    tcBitLengths[PreflateConstants::treeCodeOrderTable[i]] = tc;
  }
  HuffmanEncoder tcTree(tcBitLengths, PreflateConstants::BL_CODES, true);
  if (tcTree.error()) {
    return _error(BAD_CODE_TREE);
  }
  // unpack tree codes
  unsigned o = 0, maxo = block.nlen + block.ndist;
  for (auto i = block.treecodes.begin() + block.ncode, e = block.treecodes.end(); i != e; ++i) {
    unsigned char code = *i;
    if (code > 18) {
      return _error(BAD_LD_TREE);
    }
    tcTree.encode(_output, code);
    if (code < 16) {
      if (o >= maxo) {
        return _error(BAD_LD_TREE);
      }
      ldBitLengths[o++] = code;
      continue;
    }
    if (i + 1 == e) {
      return _error(BAD_LD_TREE);
    }
    if (code == 16 && o == 0) {
      return _error(BAD_LD_TREE);
    }
    unsigned char len = *++i;
    unsigned char tocopy = code == 16 ? ldBitLengths[o - 1] : 0;
    static unsigned char repExtraBits[3] = {2, 3, 7};
    static unsigned char repOffset[3] = {3, 3, 11};
    _output.put(len - repOffset[code - 16], repExtraBits[code - 16]);
    if (o + len > maxo) {
      return _error(BAD_LD_TREE);
    }
    memset(ldBitLengths + o, tocopy, len);
    o += len;
  }
  if (o != maxo) {
    return _error(BAD_LD_TREE);
  }
  if (!ldBitLengths[256]) {
    return _error(BAD_LD_TREE);
  }
  _dynamicLitLenEncoder = HuffmanEncoder(ldBitLengths, block.nlen, true);
  if (_dynamicLitLenEncoder.error()) {
    return _error(BAD_LD_TREE);
  }
  _litLenEncoder = &_dynamicLitLenEncoder;

  _dynamicDistEncoder = HuffmanEncoder(ldBitLengths + block.nlen, block.ndist, true);
  if (_dynamicDistEncoder.error()) {
    return _error(BAD_LD_TREE);
  }
  _distEncoder = &_dynamicDistEncoder;
  return true;
}

bool PreflateBlockReencoder::_writeTokens(const std::vector<PreflateToken>& tokens) {
  for (size_t i = 0; i < tokens.size(); ++i) {
    PreflateToken token = tokens[i];
    if (token.len == 1) {
      if (_uncompressedDataPos >= _uncompressedData.size()) {
        return _error(LITERAL_OUT_OF_BOUNDS);
      }
      unsigned char literal = _uncompressedData[_uncompressedDataPos++];
      _litLenEncoder->encode(_output, literal);
    } else {
      // handle irregular length of 258
      if (token.len == 258 + 512) {
        unsigned lencode = PreflateConstants::LCode(token.len);
        _litLenEncoder->encode(_output, PreflateConstants::L_CODES - PreflateConstants::LITERALS - 3);
        _output.put(31, 5);
        token.len -= 512;
      } else {
        unsigned lencode = PreflateConstants::LCode(token.len);
        _litLenEncoder->encode(_output, PreflateConstants::LITERALS + 1 + lencode);
        unsigned lenextra = PreflateConstants::lengthExtraTable[lencode];
        if (lenextra) {
          _output.put(token.len - PreflateConstants::MIN_MATCH - PreflateConstants::lengthBaseTable[lencode], lenextra);
        }
      }
      unsigned distcode = PreflateConstants::DCode(token.dist);
      _distEncoder->encode(_output, distcode);
      unsigned distextra = PreflateConstants::distExtraTable[distcode];
      if (distextra) {
        _output.put(token.dist - 1 - PreflateConstants::distBaseTable[distcode], distextra);
      }
      _uncompressedDataPos += token.len;
    }
  }
  _litLenEncoder->encode(_output, PreflateConstants::LITERALS);
  return true;
}

bool PreflateBlockReencoder::writeBlock(const PreflateTokenBlock& block, bool last) {
  if (status() != OK) {
    return false;
  }
  _output.put(last, 1); //
  switch (block.type) {
  case PreflateTokenBlock::DYNAMIC_HUFF:
    _output.put(2, 2); //
    _output.put(block.nlen - PreflateConstants::LITERALS - 1, 5);
    _output.put(block.ndist - 1, 5);
    _output.put(block.ncode - 4, 4);
    if (!_buildAndWriteDynamicTables(block)) {
      return false;
    }
    if (!_writeTokens(block.tokens)) {
      return false;
    }
    break;
  case PreflateTokenBlock::STATIC_HUFF:
    _output.put(1, 2); //
    _setupStaticTables();
    if (!_writeTokens(block.tokens)) {
      return false;
    }
    break;
  case PreflateTokenBlock::STORED:
    _output.put(0, 2); //
    _output.put(block.paddingBits, block.paddingBitCount);
    _output.fillByte();
    _output.put(block.uncompressedLen, 16); //
    _output.put(~block.uncompressedLen, 16); //
    if (_uncompressedDataPos + block.uncompressedLen > _uncompressedData.size()) {
      return _error(LITERAL_OUT_OF_BOUNDS);
    }
    _output.putBytes(_uncompressedData.data() + _uncompressedDataPos, block.uncompressedLen);
    _uncompressedDataPos += block.uncompressedLen;
    break;
  }
  return true;
}
void PreflateBlockReencoder::flush() {
  _output.flush();
}
