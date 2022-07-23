/**
 * @file test_additive.cc
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

#include "scl/math.h"
#include "scl/prg.h"
#include "scl/ss/additive.h"

TEST_CASE("AdditiveSS", "[ss]") {
  using FF = scl::Fp<61>;
  scl::PRG prg;

  auto secret = FF(12345);

  auto shares = scl::CreateAdditiveShares(secret, 10, prg);
  REQUIRE(shares.Size() == 10);
  REQUIRE(scl::ReconstructAdditive(shares) == secret);

  auto x = FF(55555);
  auto shr_x = scl::CreateAdditiveShares(x, 10, prg);
  auto sum = shares.Add(shr_x);

  REQUIRE(scl::ReconstructAdditive(sum) == secret + x);

  REQUIRE_THROWS_MATCHES(
      scl::CreateAdditiveShares(secret, 0, prg), std::invalid_argument,
      Catch::Matchers::Message("cannot create shares for 0 people"));
}
