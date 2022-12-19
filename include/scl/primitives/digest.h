/**
 * @file digest.h
 *
 * SCL --- Secure Computation Library
 * Copyright (C) 2022 Anders Dalskov
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

#ifndef SCL_PRIMITIVES_DIGEST_H
#define SCL_PRIMITIVES_DIGEST_H

#include <array>
#include <cstddef>
#include <iomanip>
#include <string>

#include "scl/util/str.h"

namespace scl {
namespace details {

/**
 * @brief A digest of some bitsize.
 *
 * This type is effectively <code>std::array<unsigned char, N / 8</code>.
 *
 * @tparam Bits the bitsize of the digest
 */
template <std::size_t Bits>
struct Digest {
  /**
   * @brief The actual type of a digest.
   */
  using Type = std::array<unsigned char, Bits / 8>;
};

/**
 * @brief Convert a digest to a string.
 * @param digest the digest
 * @return a hex representation of the digest.
 */
template <typename D>
std::string DigestToString(const D& digest) {
  return ToHexString(digest.begin(), digest.end());
}

}  // namespace details
}  // namespace scl

#endif  // SCL_PRIMITIVES_DIGEST_H
