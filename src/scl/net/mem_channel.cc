/* SCL --- Secure Computation Library
 * Copyright (C) 2023 Anders Dalskov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "scl/net/mem_channel.h"

#include <cstring>

// used to silence narrowing conversion errors. x will have type std::size_t
#define DIFF_T(x) static_cast<std::vector<unsigned char>::difference_type>((x))

void scl::net::MemoryBackedChannel::Send(const unsigned char* src,
                                         std::size_t n) {
  mOut->PushBack(std::vector<unsigned char>(src, src + n));
}

std::size_t scl::net::MemoryBackedChannel::Recv(unsigned char* dst,
                                                std::size_t n) {
  std::size_t rem = n;

  // if there's any leftovers from previous calls to recv, then we retrieve
  // those first.
  const auto leftovers = mOverflow.size();
  if (leftovers > 0) {
    const auto to_copy = leftovers > rem ? rem : leftovers;
    auto* data = mOverflow.data();
    std::memcpy(dst, data, to_copy);
    rem -= to_copy;
    mOverflow = std::vector<unsigned char>(mOverflow.begin() + DIFF_T(to_copy),
                                           mOverflow.end());
  }

  while (rem > 0) {
    auto data = mIn->Pop();
    const auto to_copy = data.size() > rem ? rem : data.size();
    std::memcpy(dst + (n - rem), data.data(), to_copy);
    rem -= to_copy;

    // if we didn't copy all of data, then rem == 0 and we need to save what
    // remains to overflow
    if (to_copy < data.size()) {
      const auto leftovers = data.size() - to_copy;
      const auto old_size = mOverflow.size();
      mOverflow.reserve(old_size + leftovers);
      mOverflow.insert(mOverflow.begin() + DIFF_T(old_size),
                       data.begin() + DIFF_T(to_copy),
                       data.end());
    }
  }

  return n;
}
