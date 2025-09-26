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

#include "../gf7.h"
#include "scl/math/ff.h"
#include "scl/math/mersenne127.h"
#include "scl/math/mersenne61.h"
#include "scl/math/secp256k1_field.h"
#include "scl/math/secp256k1_scalar.h"

namespace scl::test {

using Mersenne61 = FF<Mersenne61>;
using Mersenne127 = FF<Mersenne127>;
using GF7 = FF<GaloisField7>;

using Secp256k1_Field = FF<Secp256k1Field>;
using Secp256k1_Order = FF<Secp256k1Scalar>;

}  // namespace scl::test

#define FIELD_DEFS                                               \
  scl::test::Mersenne61, scl::test::Mersenne127, scl::test::GF7, \
      scl::test::Secp256k1_Field, scl::test::Secp256k1_Order
