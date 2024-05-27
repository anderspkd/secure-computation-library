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

#ifndef SCL_MATH_FIELDS_SECP256K1_FIELD_H
#define SCL_MATH_FIELDS_SECP256K1_FIELD_H

#include <array>
#include <cstddef>

#include <gmp.h>

namespace scl::math::ff {

/**
 * @brief The Field over which secp256k1 is defined.
 */
struct Secp256k1Field {
  /**
   * @brief Field elements are stored as 4 limb numbers internally.
   */
  using ValueType = std::array<mp_limb_t, 4>;

  /**
   * @brief Name of the secp256k1 field.
   */
  constexpr static const char* NAME = "secp256k1_field";

  /**
   * @brief Byte size of a secp256k1 field element.
   */
  constexpr static const std::size_t BYTE_SIZE = 4 * sizeof(mp_limb_t);

  /**
   * @brief Bit size of a secp256k1 field element.
   */
  constexpr static const std::size_t BIT_SIZE = 8 * BYTE_SIZE;
};

}  // namespace scl::math::ff

#endif  // SCL_MATH_FIELDS_SECP256K1_FIELD_H
