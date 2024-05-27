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

#ifndef SCL_MATH_FIELDS_SECP256K1_HELPERS_H
#define SCL_MATH_FIELDS_SECP256K1_HELPERS_H

#include "naf.h"
#include "scl/math/curves/secp256k1.h"
#include "scl/math/ec.h"
#include "scl/math/fields/secp256k1_field.h"
#include "scl/math/fields/secp256k1_scalar.h"

namespace scl::math::details {

/**
 * @brief Check which of two field elements is smaller.
 *
 * Used in serialization.
 */
bool isSmaller(const FF<ff::Secp256k1Field>& lhs,
               const FF<ff::Secp256k1Field>& rhs);

/**
 * @brief Compute the square root of an element.
 *
 * Used in serialization.
 */
FF<ff::Secp256k1Field> sqrt(const FF<ff::Secp256k1Field>& x);

/**
 * @brief Convert a field element out of montgomery representation.
 *
 * Used in scalar multiplications.
 */
FF<ff::Secp256k1Scalar> fromMonty(const FF<ff::Secp256k1Scalar>& x);

/**
 * @brief Convert a field element into a NAF encoding.
 *
 * Used in scalar multiplication.
 */
NafEncoding<ff::Secp256k1Scalar> toNaf(const FF<ff::Secp256k1Scalar>& x);

}  // namespace scl::math::details

#endif  // SCL_MATH_FIELDS_SECP256K1_HELPERS_H
