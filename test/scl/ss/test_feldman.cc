/**
 * @file test_feldman.cc
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

#include <catch2/catch.hpp>

#include "scl/math/curves/secp256k1.h"
#include "scl/math/ec.h"
#include "scl/primitives/prg.h"
#include "scl/ss/feldman.h"

TEST_CASE("Feldman", "[ss]") {
  using EC = scl::EC<scl::details::Secp256k1>;
  using FF = EC::Order;

  auto prg = scl::PRG::Create();
  std::size_t t = 4;

  SECTION("Share") {
    auto factory = scl::FeldmanSSFactory<EC>::Create(t, prg);
    auto secret = FF(123);
    auto sb = factory.Share(secret, 24);
    REQUIRE(sb.shares.Size() == 24);
    REQUIRE(sb.commitments.Size() == t + 1);
    REQUIRE(factory.Verify(sb.shares[22], sb.commitments, 22));
    REQUIRE(factory.Verify(secret, sb.commitments));
    REQUIRE(factory.Recover(sb.shares) == secret);
  }
}
