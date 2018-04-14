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

#include "zlib1.2.11.dec/zlib.h"
#include "zlib1.2.11.dec/prefdump.h"
#include "preflate_dumper.h"

PreflateDumper::PreflateDumper() 
  : error(false)
  , hasOpenBlock(false)
  , wbits(0)
  , hdr2(0)
  , dictid(0) {
}

PreflateDumper::~PreflateDumper() {
}

void PreflateDumper::setStreamHeader(int wlen, int zhdr2) {
  if (hasOpenBlock) {
    error = true;
    return;
  }
  wbits = wlen;
  hdr2 = zhdr2;
}
void PreflateDumper::setDictId(unsigned id) {
  if (hasOpenBlock) {
    error = true;
    return;
  }
  dictid = id;
}
void PreflateDumper::addStoredBlock(int len) {
  if (hasOpenBlock) {
    error = true;
    return;
  }
  blocks.push_back(PreflateTokenBlock(PreflateTokenBlock::STORED_X, len));
}
void PreflateDumper::addHuffBlock(bool dynamic) {
  if (hasOpenBlock) {
    error = true;
    return;
  }
  openBlock.type = dynamic ? PreflateTokenBlock::DYNAMIC_HUFF : PreflateTokenBlock::STATIC_HUFF;
  hasOpenBlock = true;
}
void PreflateDumper::addDynamicHuffLengths(int nlen, int ndist, int ncode) {
  if (!hasOpenBlock || openBlock.type != PreflateTokenBlock::DYNAMIC_HUFF) {
    error = true;
    return;
  }
  openBlock.setHuffLengths(nlen, ndist, ncode);
}
void PreflateDumper::addDynamicHuffTreeCode(int code) {
  if (!hasOpenBlock || openBlock.type != PreflateTokenBlock::DYNAMIC_HUFF) {
    error = true;
    return;
  }
  openBlock.addTreeCode(code);
}
void PreflateDumper::addLiteral() {
  if (!hasOpenBlock) {
    error = true;
    return;
  }
  openBlock.tokens.push_back(PreflateToken(PreflateToken::LITERAL));
}
void PreflateDumper::addReference(int dist, int len) {
  if (!hasOpenBlock) {
    error = true;
    return;
  }
  openBlock.tokens.push_back(PreflateToken(PreflateToken::REFERENCE, len, dist));
}
void PreflateDumper::addEOB() {
  if (hasOpenBlock) {
    blocks.push_back(std::move(openBlock));
    hasOpenBlock = false;
  }
}

void PreflateDumper::addUncompressedData(const unsigned char* data, const unsigned len) {
  uncompressed.insert(uncompressed.end(), data, data + len);
}

// ------------------------------------

void prefdump_stream_header(void* dumper, int wlen, int zhdr2) {
  if (dumper) {
    ((PreflateDumper*)dumper)->setStreamHeader(wlen, zhdr2);
  }
}
void prefdump_dictid(void* dumper, uLong dictid) {
  if (dumper) {
    ((PreflateDumper*)dumper)->setDictId(dictid);
  }
}
void prefdump_new_stored_block(void* dumper, int len) {
  if (dumper) {
    ((PreflateDumper*)dumper)->addStoredBlock(len);
  }
}
void prefdump_new_huff_block(void* dumper, int dynamic) {
  if (dumper) {
    ((PreflateDumper*)dumper)->addHuffBlock(dynamic != 0);
  }
}
void prefdump_dyn_huff_lengths(void* dumper, int nlen, int ndist, int ncode) {
  if (dumper) {
    ((PreflateDumper*)dumper)->addDynamicHuffLengths(nlen, ndist, ncode);
  }
}
void prefdump_dyn_huff_tcode(void* dumper, int code) {
  if (dumper) {
    ((PreflateDumper*)dumper)->addDynamicHuffTreeCode(code);
  }
}
void prefdump_literal(void* dumper) {
  if (dumper) {
    ((PreflateDumper*)dumper)->addLiteral();
  }
}
void prefdump_reference(void* dumper, int dist, int len) {
  if (dumper) {
    ((PreflateDumper*)dumper)->addReference(dist, len);
  }
}
void prefdump_eob(void* dumper) {
  if (dumper) {
    ((PreflateDumper*)dumper)->addEOB();
  }
}
