/**
 * @file secp256k1.h
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

#ifndef SCL_MATH_CURVES_SECP256K1_H
#define SCL_MATH_CURVES_SECP256K1_H

#include <gmp.h>

#include "scl/math/ec.h"
#include "scl/math/ff.h"

namespace scl {
namespace details {

/**
 * @brief Secp256k1 elliptic curve definition.
 */
struct Secp256k1 {
  /**
   * @brief The Field over which secp256k1 is defined.
   */
  struct Field {
    /**
     * @brief Field elements are stored as 4 limb numbers internally.
     */
    using ValueType = std::array<mp_limb_t, 4>;

    /**
     * @brief Name of the secp256k1 field.
     */
    constexpr static const char* kName = "secp256k1_field";

    /**
     * @brief Byte size of a secp256k1 field element.
     */
    constexpr static const std::size_t kByteSize = 4 * sizeof(mp_limb_t);

    /**
     * @brief Bit size of a secp256k1 field element.
     */
    constexpr static const std::size_t kBitSize = 8 * kByteSize;
  };

  /**
   * @brief Secp256k1 curve elements are stored in projective coordinates.
   */
  using ValueType = std::array<scl::FF<Field>, 3>;

  /**
   * @brief Name of the secp256k1 curve.
   */
  constexpr static const char* kName = "secp256k1";
};

}  // namespace details
}  // namespace scl

#endif  // SCL_MATH_CURVES_SECP256K1_H
