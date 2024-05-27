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

#ifndef SCL_MATH_CURVES_SECP256K1_H
#define SCL_MATH_CURVES_SECP256K1_H

#include <gmp.h>

#include "scl/math/curves/ec_ops.h"
#include "scl/math/ff.h"
#include "scl/math/fields/secp256k1_field.h"
#include "scl/math/fields/secp256k1_scalar.h"

namespace scl::math::ec {

/**
 * @brief Elliptic curve definition for secp256k1.
 * @see http://www.secg.org/sec2-v2.pdf
 */
struct Secp256k1 {
  /**
   * @brief The finite field defined by
   * \f$p=2^{256}-2^{32}-2^{9}-2^{8}-2^{7}-2^{6}-2^{4}-1\f$
   */
  using Field = ff::Secp256k1Field;

  /**
   * @brief The finite field defined by a large prime order subgroup.
   */
  using Scalar = ff::Secp256k1Scalar;

  /**
   * @brief Secp256k1 curve elements are stored in projective coordinates.
   */
  using ValueType = std::array<FF<Field>, 3>;

  /**
   * @brief Name of the secp256k1 curve.
   */
  constexpr static const char* NAME = "secp256k1";
};

}  // namespace scl::math::ec

#endif  // SCL_MATH_CURVES_SECP256K1_H
