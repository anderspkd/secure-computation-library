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

#include "../gf7.h"
#include "scl/math/curves/secp256k1.h"
#include "scl/math/fp.h"

namespace scl::test {

using Mersenne61 = math::Fp<61>;
using Mersenne127 = math::Fp<127>;
using GF7 = math::FF<GaloisField7>;

#ifdef SCL_ENABLE_EC_TESTS
using Secp256k1_Field = math::FF<math::Secp256k1::Field>;
using Secp256k1_Order = math::FF<math::Secp256k1::Order>;
#endif

}  // namespace scl::test

#ifdef SCL_ENABLE_EC_TESTS
#define FIELD_DEFS                                               \
  scl::test::Mersenne61, scl::test::Mersenne127, scl::test::GF7, \
      scl::test::Secp256k1_Field, scl::test::Secp256k1_Order
#else
#define FIELD_DEFS scl::test::Mersenne61, scl::test::Mersenne127, scl::test::GF7
#endif
