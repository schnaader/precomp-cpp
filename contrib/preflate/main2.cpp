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
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <vector>

#include "preflate_checker.h"
#include "preflate_decoder.h"
#include "preflate_info.h"
#include "preflate_reencoder.h"
#include "support/support_tests.h"

bool loadfile(
    std::vector<unsigned char>& content, 
    const std::string& fn) {
  FILE* f = fopen(fn.c_str(), "rb");
  if (!f) {
    return false;
  }
  fseek(f, 0, SEEK_END);
  long length = ftell(f);
  fseek(f, 0, SEEK_SET);
  content.resize(length);
  long read = fread(content.data(), 1, content.size(), f);
  fclose(f);
  return read == length;
}
bool savefile(
  const std::vector<unsigned char>& content,
  const std::string& fn) {
  FILE* f = fopen(fn.c_str(), "wb");
  if (!f) {
    return false;
  }
  size_t written = fwrite(content.data(), 1, content.size(), f);
  fclose(f);
  return written == content.size();
}

int test(const char* const * const fns, const unsigned fncnt) {
  bool ok = true;
  for (unsigned i = 0; i < fncnt; ++i) {
    std::vector<unsigned char> content;
    if (!loadfile(content, fns[i])) {
      printf("loading of %s failed\n", fns[i]);
      ok = false;
    } else {
      std::vector<unsigned char> unpacked;
      std::vector<unsigned char> recon;
      bool check_ok = preflate_decode(unpacked, recon, content, 1 << 18);
      if (check_ok) {
        std::vector<unsigned char> rebuilt_content;
        check_ok = preflate_reencode(rebuilt_content, recon, unpacked);
        if (check_ok) {
          check_ok = content == rebuilt_content;
          if (check_ok) {
            printf("splitting & recombining %s successful (%d -> %d + %d)\n",
                   fns[i], (int)content.size(), (int)unpacked.size(), (int)recon.size());
          } else {
            printf("splitting & recombining %s failed: output not bitexact (%d -> %d + %d)\n",
                   fns[i], (int)content.size(), (int)unpacked.size(), (int)recon.size());
          }
        } else {
          printf("recombining %s failed (%d -> %d + %d)\n",
                 fns[i], (int)content.size(), (int)unpacked.size(), (int)recon.size());
        }
      } else {
        printf("splitting %s failed\n", fns[i]);
      }
      ok = ok && check_ok;
    }
  }
  if (ok) {
    printf("All checks ok\n");
  }
  return ok ? 0 : -1;
}
int split(const char* const * const fns, const unsigned fncnt) {
  bool ok = true;
  for (unsigned i = 0; i < fncnt; ++i) {
    std::vector<unsigned char> content;
    if (!loadfile(content, fns[i])) {
      printf("loading of %s failed\n", fns[i]);
      ok = false;
    } else {
      std::vector<unsigned char> unpacked;
      std::vector<unsigned char> recon;
      bool check_ok = preflate_decode(unpacked, recon, content);
      if (check_ok) {
        savefile(unpacked, std::string(fns[i]) + ".u");
        savefile(recon, std::string(fns[i]) + ".r");
        printf("splitting %s successful (%d -> %d + %d)\n", 
               fns[i], (int)content.size(), (int)unpacked.size(), (int)recon.size());
      } else {
        printf("splitting %s failed\n", fns[i]);
      }
      ok = ok && check_ok;
    }
  }
  if (ok) {
    printf("All ok\n");
  }
  return ok ? 0 : -1;
}
int combine(const char* const * const fns, const unsigned fncnt, const std::string& ext) {
  bool ok = true;
  for (unsigned i = 0; i < fncnt; ++i) {
    std::vector<unsigned char> unpacked;
    std::vector<unsigned char> recon;
    if (!loadfile(unpacked, std::string(fns[i]) + ".u")
        || !loadfile(recon, std::string(fns[i]) + ".r")) {
      printf("loading of %s.u/.r failed\n", fns[i]);
      ok = false;
    } else {
      std::vector<unsigned char> content;
      bool check_ok = preflate_reencode(content, recon, unpacked);
      if (check_ok) {
        savefile(content, std::string(fns[i]) + ext);
        printf("recombining %s%s successful (%d + %d -> %d)\n",
               fns[i], ext.c_str(), (int)unpacked.size(), (int)recon.size(), (int)content.size());
      } else {
        printf("recombining %s%s failed\n", fns[i], ext.c_str());
      }
      ok = ok && check_ok;
    }
  }
  if (ok) {
    printf("All ok\n");
  }
  return ok ? 0 : -1;
}


#include "preflate_seq_chain.h"
int main(int argc, const char * const * const argv) {
  puts("preflate v0.3.2");
  if (argc >= 3) {
    if (!strcmp(argv[1], "-t")) {
      const char* const * fns = argv + 2;
      size_t fncnt = argc - 2;
      return test(fns, fncnt);
    }
    if (!strcmp(argv[1], "-s")) {
      return split(argv + 2, argc - 2);
    }
    if (!strcmp(argv[1], "-r")) {
      return combine(argv + 2, argc - 2, "");
    }
    if (!strcmp(argv[1], "-x")) {
      return combine(argv + 2, argc - 2, ".x");
    }
  }
  printf("usage: %s -t FILE [FILE ... FILE]\n", argv[0]);
  printf("       test uncompression and recompression\n");
  printf("       %s -s FILE [FILE ... FILE]\n", argv[0]);
  printf("       split deflate stream (FILE) into uncompressed part\n");
  printf("       (FILE.u) and reconstruction info (FILE.r)\n");
  printf("       %s -r FILE [FILE ... FILE]\n", argv[0]);
  printf("       recombine uncompressed part (FILE.u) and reconstruction\n");
  printf("       info (FILE.r) into deflate stream (FILE)\n");
  printf("       %s -x FILE [FILE ... FILE]\n", argv[0]);
  printf("       recombines into FILE.x instead of FILE\n");
  return -1;
}
