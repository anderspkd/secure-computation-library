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

#include "scl/math/ff.h"
#include "scl/math/mersenne61.h"
#include "scl/primitives/prg.h"
#include "scl/ss/additive.h"

using namespace scl;

TEST_CASE("AdditiveSS", "[ss]") {
  using Elem = FF<Mersenne61>;
  auto prg = PRG::create();

  auto secret = Elem(12345);

  auto shares = additiveShare(secret, 10, prg);
  REQUIRE(shares.size() == 10);
  REQUIRE(shares.sum() == secret);

  auto x = Elem(55555);
  auto shr_x = additiveShare(x, 10, prg);
  auto share_sum = shares.add(shr_x);

  REQUIRE(share_sum.sum() == secret + x);
}
