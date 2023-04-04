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

#include "scl/math/fp.h"
#include "scl/ss/additive.h"
#include "scl/util/prg.h"

using namespace scl;

TEST_CASE("AdditiveSS", "[ss]") {
  using FF = math::Fp<61>;
  auto prg = util::PRG::Create();

  auto secret = FF(12345);

  auto shares = ss::AdditiveShare(secret, 10, prg);
  REQUIRE(shares.Size() == 10);
  REQUIRE(shares.Sum() == secret);

  auto x = FF(55555);
  auto shr_x = ss::AdditiveShare(x, 10, prg);
  auto sum = shares.Add(shr_x);

  REQUIRE(sum.Sum() == secret + x);
}
