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

#ifndef PREFLATE_INPUT_H
#define PREFLATE_INPUT_H

#include <vector>

class PreflateInput {
public:
  PreflateInput(const std::vector<unsigned char>& v)
    : _data(v.size() > 0 ? &v[0] : nullptr), _size(v.size()), _pos(0) {}

  const unsigned pos() const {
    return _pos;
  }

  const unsigned char* curChars(int offset = 0) const {
    return _data + _pos + offset;
  }
  const unsigned char curChar(int offset = 0) const {
    return _data[_pos + offset];
  }
  void advance(const unsigned l) {
    _pos += l;
  }
  const unsigned remaining() const {
    return _size - _pos;
  }

private:
  const unsigned char* _data;
  unsigned _size;
  unsigned _pos;
};


#endif /* PREFLATE_INPUT_H */
