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

#ifndef SCL_MATH_SECP256K1_HELPERS_H
#define SCL_MATH_SECP256K1_HELPERS_H

#include "scl/math/curves/secp256k1.h"
#include "scl/math/ec.h"

namespace scl::math {

/**
 * @brief Helper class for Secp256k1::Field.
 */
template <>
struct FFAccess<Secp256k1::Field> {
  /**
   * @brief Compare two field elements lexicographical.
   */
  static bool IsSmaller(const FF<Secp256k1::Field>& lhs,
                        const FF<Secp256k1::Field>& rhs);

  /**
   * @brief Compute the square root of an element
   */
  static FF<Secp256k1::Field> ComputeSqrt(const FF<Secp256k1::Field>& x);
};

/**
 * @brief Helper class for Secp256k1::Order.
 */
template <>
struct FFAccess<Secp256k1::Scalar> {
  /**
   * @brief Convert a field element out of montgomery representation.
   */
  static FF<Secp256k1::Scalar> FromMonty(const FF<Secp256k1::Scalar>& element);

  /**
   * @brief Find the position of the highest set bit.
   */
  static std::size_t HigestSetBit(const FF<Secp256k1::Scalar>& element);

  /**
   * @brief Check if a particular bit is set.
   *
   * \p pos is assumed to be at or below HighestSetBit(\p element).
   */
  static bool TestBit(const FF<Secp256k1::Scalar>& element, std::size_t pos);
};

}  // namespace scl::math

#endif  // SCL_MATH_SECP256K1_HELPERS_H
