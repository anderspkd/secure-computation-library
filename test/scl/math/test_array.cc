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

#include "scl/math/array.h"
#include "scl/math/ec.h"
#include "scl/math/ff.h"
#include "scl/math/mersenne127.h"
#include "scl/math/secp256k1.h"
#include "scl/serialization.h"

using namespace scl;

using G = EC<Secp256k1>;
using F = G::ScalarField;

TEST_CASE("Array default init", "[math]") {
  const G inf;
  Array<G, 4> p;

  REQUIRE(p == Array<G, 4>{{inf, inf, inf, inf}});

  const auto zero = F::zero();
  Array<F, 3> q;
  REQUIRE(q == Array<F, 3>{{zero, zero, zero}});
}

TEST_CASE("Array operations", "[math]") {
  Array<F, 3> p = {{F(1), F(2), F(4)}};
  Array<F, 3> q = {{F(4), F(2), F(1)}};

  REQUIRE(p + q == Array<F, 3>{{F(5), F(4), F(5)}});
  REQUIRE(p - q == Array<F, 3>{{F(-3), F(0), F(3)}});
  REQUIRE(p * q == Array<F, 3>{{F(4), F(4), F(4)}});
  REQUIRE(q * p == Array<F, 3>{{F(4), F(4), F(4)}});
}

TEST_CASE("Array operations mixed", "[math]") {
  const auto gen = G::generator();
  Array<G, 3> g = {{gen, gen, gen}};
  Array<F, 3> f = {{F(44), F(55), F(66)}};

  REQUIRE(g * f == Array<G, 3>{{gen * F(44), gen * F(55), gen * F(66)}});
  REQUIRE(f * g == Array<G, 3>{{gen * F(44), gen * F(55), gen * F(66)}});
}

TEST_CASE("Array to string", "[math]") {
  Array<G, 2> p;
  REQUIRE(p.toString() == "P{EC{POINT_AT_INFINITY}, EC{POINT_AT_INFINITY}}");
}

TEST_CASE("Array serialization", "[math]") {
  auto prg = PRG::create("prod seri");
  auto prod = Array<F, 3>::random(prg);

  unsigned char buf[Serializer<Array<F, 3>>::sizeOf(prod)];
  Serializer<Array<F, 3>>::write(prod, buf);

  Array<F, 3> p;

  REQUIRE(p != prod);

  Serializer<Array<F, 3>>::read(p, buf);

  REQUIRE(p == prod);
}
