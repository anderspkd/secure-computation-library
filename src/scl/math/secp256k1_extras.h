/**
 * @file secp256k1_extras.h
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

#ifndef SCL_MATH_SECP256K1_EXTRAS_H
#define SCL_MATH_SECP256K1_EXTRAS_H

#include "scl/math/curves/secp256k1.h"
#include "scl/math/ec.h"

namespace scl {

// Helper class for point serialization. It provides a means to compute square
// roots of field elements as well as compare them lexicographical, both of
// which are needed when reading/writing compressed points.
template <>
struct SCL_FF_Extras<details::Secp256k1::Field> {
  // compare two field elements and returns true if lhs < rhs.
  static bool IsSmaller(const scl::FF<details::Secp256k1::Field>& lhs,
                        const scl::FF<details::Secp256k1::Field>& rhs);

  // Compute the square root of x
  static FF<details::Secp256k1::Field> ComputeSqrt(
      const scl::FF<details::Secp256k1::Field>& x);
};

template <>
struct SCL_FF_Extras<details::Secp256k1::Order> {
  // Get position of the highest set bit
  static std::size_t HigestSetBit(
      const scl::FF<details::Secp256k1::Order>& element);

  // Check if a particular bit is set. The position is assumed to be at or below
  // <code>HigestSetBit(element)</code>.
  static bool TestBit(const scl::FF<details::Secp256k1::Order>& element,
                      std::size_t pos);
};

}  // namespace scl

#endif  // SCL_MATH_SECP256K1_EXTRAS_H
