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

#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

#include "scl/math/curves/secp256k1.h"
#include "scl/math/ec.h"
#include "scl/ss/feldman.h"
#include "scl/ss/shamir.h"
#include "scl/util/prg.h"

using namespace scl;

using EC = math::EC<math::ec::Secp256k1>;
using FF = EC::ScalarField;

TEST_CASE("Feldman", "[ss]") {
  auto prg = util::PRG::create("feldman");
  std::size_t t = 4;

  auto secret = FF(123);
  auto sb = ss::feldmanSecretShare<EC>(secret, 4, 24, prg);
  REQUIRE(sb.commitments[0] == secret * EC::generator());
  REQUIRE(sb.shares.size() == 24);
  REQUIRE(sb.commitments.size() == t + 1);
  REQUIRE(ss::feldmanVerify<EC>({secret, sb.commitments}, 0));
  REQUIRE(ss::feldmanVerify<EC>(secret, sb.commitments, 0));
  REQUIRE(ss::feldmanVerify(sb.getShare(22), 23));
  REQUIRE(ss::shamirRecoverP(sb.shares.subVector(5)) == secret);
}

TEST_CASE("Feldman hom", "[ss]") {
  auto prg = util::PRG::create("feldman hom");
  std::size_t t = 4;

  auto s0 = FF(123);
  auto s1 = FF(44);

  auto ss0 = ss::feldmanSecretShare<EC>(s0, t, 10, prg);
  auto ss1 = ss::feldmanSecretShare<EC>(s1, t, 10, prg);

  auto ss2 = ss0.shares.add(ss1.shares);
  auto com2 = ss0.commitments.add(ss1.commitments);

  // Check that new commitment works for the sum of the secrets.
  REQUIRE(ss::feldmanVerify<EC>({s0 + s1, com2}, 0));
  // Check that new commitment works for an individual share.
  REQUIRE(ss::feldmanVerify<EC>({ss2[5], com2}, 6));
}
