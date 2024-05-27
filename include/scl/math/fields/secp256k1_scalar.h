/* SCL --- Secure Computation Library
 * Copyright (C) 2024 Anders Dalskov
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

#ifndef SCL_MATH_FIELDS_SECP256K1_SCALAR_H
#define SCL_MATH_FIELDS_SECP256K1_SCALAR_H

#include <array>
#include <cstddef>

#include <gmp.h>

namespace scl::math::ff {

/**
 * @brief Finite field modulo a Secp256k1 prime order sub-group.
 */
struct Secp256k1Scalar {
  /**
   * @brief Internal type of elements.
   */
  using ValueType = std::array<mp_limb_t, 4>;

  /**
   * @brief Name of the field.
   */
  constexpr static const char* NAME = "secp256k1_order";

  /**
   * @brief Size of an element in bytes.
   */
  constexpr static const std::size_t BYTE_SIZE = 4 * sizeof(mp_limb_t);

  /**
   * @brief Size of an element in bits.
   */
  constexpr static const std::size_t BIT_SIZE = 8 * BYTE_SIZE;
};

}  // namespace scl::math::ff

#endif  // SCL_MATH_FIELDS_SECP256K1_SCALAR_H
