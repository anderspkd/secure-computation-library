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

#ifndef SCL_UTIL_DIGEST_H
#define SCL_UTIL_DIGEST_H

#include <array>
#include <cstddef>
#include <string>

#include "scl/util/str.h"

namespace scl::util {

/**
 * @brief A digest of some bitsize.
 * @tparam Bits the bitsize of the digest
 *
 * This type is effectively <code>std::array<unsigned char, N / 8</code>.
 */
template <std::size_t Bits>
using Digest = std::array<unsigned char, Bits / 8>;

/**
 * @brief Convert a digest to a string.
 * @param digest the digest
 * @return a hex representation of the digest.
 */
template <typename D>
std::string DigestToString(const D& digest) {
  return ToHexString(digest.begin(), digest.end());
}

}  // namespace scl::util

#endif  // SCL_UTIL_DIGEST_H
