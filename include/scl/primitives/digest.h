/* SCL --- Secure Computation Library
 * Copyright (C) 2025 Anders Dalskov
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

#pragma once

#include <array>
#include <cstddef>
#include <string>

#include "scl/hex.h"

namespace scl {

/**
 * @brief A digest of some bitsize.
 */
template <std::size_t BITS>
using Digest = std::array<unsigned char, BITS / 8>;

/**
 * @brief Convert a digest to a hex string.
 */
template <typename DIGEST>
std::string digestToString(const DIGEST& digest) {
  return details::toHexString(digest.begin(), digest.end());
}

}  // namespace scl
