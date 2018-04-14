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

#ifndef ARRAY_HELPER_H
#define ARRAY_HELPER_H

unsigned sumArray(const unsigned* data, const unsigned n);

template <unsigned N>
inline unsigned sumArray(const unsigned (&data)[N]) {
  return sumArray(data, N);
}

#endif /* ARRAY_HELPER_H */
