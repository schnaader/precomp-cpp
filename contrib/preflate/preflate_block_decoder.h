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

#ifndef PREFLATE_BLOCK_DECODER_H
#define PREFLATE_BLOCK_DECODER_H

#include "preflate_constants.h"
#include "preflate_hash_chain.h"
#include "preflate_input.h"
#include "preflate_token.h"
#include "support/bitstream.h"
#include "support/huffman_decoder.h"
#include "support/outputcachestream.h"

class PreflateBlockDecoder {
public:
  enum ErrorCode {
    OK,
    STORED_BLOCK_LEN_MISMATCH,
    STORED_BLOCK_PADDING_MISMATCH,
    BADLY_CODED_MAX_LENGTH
  };
  PreflateBlockDecoder(BitInputStream& input,
                       OutputCacheStream& output);

  bool readBlock(PreflateTokenBlock&, bool& last);
  ErrorCode status() const {
    return _errorCode;
  }

private:
  bool _error(const ErrorCode);

  unsigned char _readBit() {
    return _input.get(1);
  }
  unsigned _readBits(const unsigned bits) {
    return _input.get(bits);
  }
  void _skipToByte() {
    _input.skipToByte();
  }
  bool _checkLastBitsOfByte() {
    return _input.checkLastBitsOfByteAreZero();
  }
  void _writeLiteral(const unsigned char l) {
    _output.write(&l, 1);
  }
  void _writeReference(const size_t dist, const size_t len) {
    _output.reserve(len);
    if (len <= dist) {
      _output.write(_output.cacheEnd() - dist, len);
    } else {
      const uint8_t* ptr = _output.cacheEnd() - dist;
      for (size_t i = 0; i < len; ++i) {
        _output.write(&ptr[i], 1);
      }
    }
  }
  void _setupStaticTables();
  bool _readDynamicTables(PreflateTokenBlock&);

  BitInputStream& _input;
  OutputCacheStream& _output;
  ErrorCode _errorCode;
  const HuffmanDecoder* _litLenDecoder;
  const HuffmanDecoder* _distDecoder;
  HuffmanDecoder _dynamicLitLenDecoder;
  HuffmanDecoder _dynamicDistDecoder;
};

#endif /* PREFLATE_BLOCK_DECODER_H */
