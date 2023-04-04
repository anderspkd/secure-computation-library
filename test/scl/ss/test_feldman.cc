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

#include <catch2/catch.hpp>
#include <stdexcept>

#include "scl/math/curves/secp256k1.h"
#include "scl/math/ec.h"
#include "scl/ss/feldman.h"
#include "scl/ss/shamir.h"
#include "scl/util/prg.h"

using namespace scl;

TEST_CASE("Feldman", "[ss]") {
  using EC = math::EC<math::Secp256k1>;
  using FF = EC::Order;

  auto prg = util::PRG::Create();
  std::size_t t = 4;

  auto secret = FF(123);
  auto sb = ss::FeldmanShare<EC>(secret, 4, 24, prg);
  REQUIRE(sb.shares.Size() == 24);
  REQUIRE(sb.commitments.Size() == t + 1);
  REQUIRE(ss::FeldmanVerify({0, secret}, sb.commitments));
  REQUIRE(ss::FeldmanVerify({23, sb.shares[22]}, sb.commitments));
  REQUIRE(ss::ShamirRecoverP(sb.shares.SubVector(5)) == secret);
}
