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

#include <catch2/catch_test_macros.hpp>

#include "scl/math/ec.h"
#include "scl/math/secp256k1.h"
#include "scl/primitives/prg.h"
#include "scl/ss/feldman.h"
#include "scl/ss/shamir.h"

using namespace scl;

using Curve = EC<Secp256k1>;
using Field = Curve::ScalarField;

TEST_CASE("Feldman", "[ss]") {
  auto prg = PRG::create("feldman");
  std::size_t t = 4;

  auto secret = Field(123);
  auto sb = createFeldmanVerifiableSharing<Curve>(secret, 4, 24, prg);
  REQUIRE(sb.commitments[0] == secret * Curve::generator());
  REQUIRE(sb.shares.size() == 24);
  REQUIRE(sb.commitments.size() == t + 1);
  REQUIRE(feldmanVerify<Curve>({secret, sb.commitments}, 0));
  REQUIRE(feldmanVerify<Curve>({sb.shares[22], sb.commitments}, 23));
  REQUIRE(shamirRecoverP(sb.shares.subVector(5)) == secret);
}

TEST_CASE("Feldman hom", "[ss]") {
  auto prg = PRG::create("feldman hom");
  std::size_t t = 4;

  auto s0 = Field(123);
  auto s1 = Field(44);

  auto ss0 = createFeldmanVerifiableSharing<Curve>(s0, t, 10, prg);
  auto ss1 = createFeldmanVerifiableSharing<Curve>(s1, t, 10, prg);

  auto ss2 = ss0.shares.add(ss1.shares);
  auto com2 = ss0.commitments.add(ss1.commitments);

  // Check that new commitment works for the sum of the secrets.
  REQUIRE(feldmanVerify<Curve>({s0 + s1, com2}, 0));
  // Check that new commitment works for an individual share.
  REQUIRE(feldmanVerify<Curve>({ss2[5], com2}, 6));
}
