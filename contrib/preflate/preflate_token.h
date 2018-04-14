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

#ifndef PREFLATE_TOKEN_H
#define PREFLATE_TOKEN_H

#include <stdint.h>
#include <vector>

/* len: 1 for literal, >= 3 for reference */
struct PreflateToken {
  enum typeLit {
    LITERAL
  };
  enum typeRef {
    REFERENCE
  };
  enum typeNon {
    NONE
  };
  unsigned short len;
  unsigned short dist;

  PreflateToken(typeNon n) : len(0), dist(0) {}
  PreflateToken(typeLit l) : len(1), dist(0) {}
  PreflateToken(typeRef r, unsigned short l, unsigned short d) : len(l), dist(d) {}
};

struct PreflateTokenBlock {
  enum Type {
    STORED, DYNAMIC_HUFF, STATIC_HUFF
  };
  enum StoredBlockType {
    STORED_X
  };
  enum HuffBlockType {
    DYNAMIC_HUFF_X, STATIC_HUFF_X
  };

  Type type;
  uint64_t uncompressedStartPos;
  uint64_t uncompressedLen;
  int32_t contextLen; // prefix size required to handle all references
  unsigned short nlen, ndist, ncode;
  std::vector<unsigned char> treecodes;
  std::vector<PreflateToken> tokens;

  PreflateTokenBlock()
    : type(STORED)
    , uncompressedLen(0) {}
  PreflateTokenBlock(StoredBlockType, int len_)
    : type(STORED)
    , uncompressedLen(len_) {}
  PreflateTokenBlock(HuffBlockType t)
    : type(t == DYNAMIC_HUFF_X ? DYNAMIC_HUFF : STATIC_HUFF)
    , uncompressedLen(0) {}
  void setHuffLengths(int nlen_, int ndist_, int ncode_) {
    nlen = nlen_;
    ndist = ndist_;
    ncode = ncode_;
  }
  void addTreeCode(int code) {
    treecodes.push_back(code);
  }
  void addToken(const PreflateToken& token) {
    tokens.push_back(token);
  }
};

bool isEqual(const PreflateTokenBlock&, const PreflateTokenBlock&);


#endif /* PREFLATE_TOKEN_H */
