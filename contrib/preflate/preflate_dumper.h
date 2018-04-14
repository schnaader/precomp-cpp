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

#ifndef PREFLATE_DUMPER_H
#define PREFLATE_DUMPER_H

#include <vector>
#include "preflate_token.h"

class PreflateDumper {
public:
  PreflateDumper();
  ~PreflateDumper();

  void setStreamHeader(int wlen, int zhdr2);
  void setDictId(unsigned id);
  void addStoredBlock(int len);
  void addHuffBlock(bool dynamic);
  void addDynamicHuffLengths(int nlen, int ndist, int ncode);
  void addDynamicHuffTreeCode(int code);
  void addLiteral();
  void addReference(int dist, int len);
  void addEOB();
  void addUncompressedData(const unsigned char*, const unsigned);

  const std::vector<PreflateTokenBlock>& getBlocks() const {
    return blocks;
  }
  bool hadErrors() const {
    return error;
  }
  const std::vector<unsigned char>& uncompressedData() const {
    return uncompressed;
  }

//private:
  bool error;
  bool hasOpenBlock;
  PreflateTokenBlock openBlock;
  unsigned char wbits, hdr2;
  unsigned dictid;
  std::vector<PreflateTokenBlock> blocks;
  std::vector<unsigned char> uncompressed;
};

#endif // PREFLATE_DUMPER_H
