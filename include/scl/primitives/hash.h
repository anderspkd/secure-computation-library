/**
 * @file hash.h
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

#ifndef SCL_PRIMITIVES_HASH_H
#define SCL_PRIMITIVES_HASH_H

#include <cstddef>

#include "scl/primitives/sha3.h"

namespace scl {

/**
 * @brief A default hash function given a digest size.
 *
 * This type defults to one of the three instantiations of SHA3 that SCL
 * provides.
 */
template <std::size_t DigestSize>
using Hash = details::Sha3<DigestSize>;

}  // namespace scl

#endif  // SCL_PRIMITIVES_HASH_H
