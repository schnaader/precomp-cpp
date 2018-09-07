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

#ifndef PREFLATE_BLOCK_REENCODER_H
#define PREFLATE_BLOCK_REENCODER_H

#include "preflate_constants.h"
#include "preflate_token.h"
#include "support/bitstream.h"
#include "support/huffman_encoder.h"

class PreflateBlockReencoder {
public:
  enum ErrorCode {
    OK,
    LITERAL_OUT_OF_BOUNDS,
    TREE_OUT_OF_RANGE,
    BAD_CODE_TREE,
    BAD_LD_TREE,
  };
  /*  enum {
    BUFSIZE = 1024
  };

  std::vector<unsigned char> output;
  unsigned char buffer[BUFSIZE];
  unsigned bufferpos;
  unsigned bitbuffer;
  unsigned bitbuffersize;*/

/*  unsigned short litLenDistCodeStorage[PreflateConstants::LD_CODES];
  unsigned short treeCodeStorage[PreflateConstants::BL_CODES];
  unsigned char litLenDistBitStorage[PreflateConstants::LD_CODES];
  unsigned char treeBitStorage[PreflateConstants::BL_CODES];
  const unsigned short *litLenCode, *distCode, *treeCode;
  const unsigned char *litLenBits, *distBits, *treeBits;*/

  PreflateBlockReencoder(BitOutputStream& bos, 
                         const std::vector<unsigned char>& uncompressedData,
                         const size_t uncompressedOffset);
  bool writeBlock(const PreflateTokenBlock&, const bool last);
  void flush();

  ErrorCode status() const {
    return _errorCode;
  }

private:
  bool _error(const ErrorCode);

  void _setupStaticTables();
  bool _buildAndWriteDynamicTables(const PreflateTokenBlock&);
  bool _writeTokens(const std::vector<PreflateToken>& tokens);

  BitOutputStream& _output;
  const std::vector<unsigned char>& _uncompressedData;
  size_t _uncompressedDataPos;
  ErrorCode _errorCode;

  const HuffmanEncoder* _litLenEncoder;
  const HuffmanEncoder* _distEncoder;
  HuffmanEncoder _dynamicLitLenEncoder;
  HuffmanEncoder _dynamicDistEncoder;
};

#endif /* PREFLATE_BLOCK_REENCODER_H */
