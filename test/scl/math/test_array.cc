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

#include "scl/math/array.h"
#include "scl/math/curves/secp256k1.h"
#include "scl/math/ec.h"
#include "scl/math/ff.h"
#include "scl/math/fields/mersenne127.h"
#include "scl/serialization/serializer.h"

using namespace scl;
using G = math::EC<math::ec::Secp256k1>;
using F = G::ScalarField;

TEST_CASE("Array default init", "[math]") {
  const G inf;
  math::Array<G, 4> p;

  REQUIRE(p == math::Array<G, 4>{{inf, inf, inf, inf}});

  const auto zero = F::zero();
  math::Array<F, 3> q;
  REQUIRE(q == math::Array<F, 3>{{zero, zero, zero}});
}

TEST_CASE("Array operations", "[math]") {
  math::Array<F, 3> p = {{F(1), F(2), F(4)}};
  math::Array<F, 3> q = {{F(4), F(2), F(1)}};

  REQUIRE(p + q == math::Array<F, 3>{{F(5), F(4), F(5)}});
  REQUIRE(p - q == math::Array<F, 3>{{F(-3), F(0), F(3)}});
  REQUIRE(p * q == math::Array<F, 3>{{F(4), F(4), F(4)}});
  REQUIRE(q * p == math::Array<F, 3>{{F(4), F(4), F(4)}});
}

TEST_CASE("Array operations mixed", "[math]") {
  const auto gen = G::generator();
  math::Array<G, 3> g = {{gen, gen, gen}};
  math::Array<F, 3> f = {{F(44), F(55), F(66)}};

  REQUIRE(g * f == math::Array<G, 3>{{gen * F(44), gen * F(55), gen * F(66)}});
  REQUIRE(f * g == math::Array<G, 3>{{gen * F(44), gen * F(55), gen * F(66)}});
}

TEST_CASE("Array to string", "[math]") {
  math::Array<G, 2> p;
  REQUIRE(p.toString() == "P{EC{POINT_AT_INFINITY}, EC{POINT_AT_INFINITY}}");
}

TEST_CASE("Array serialization", "[math]") {
  auto prg = util::PRG::create("prod seri");
  auto prod = math::Array<F, 3>::random(prg);

  using S = seri::Serializer<math::Array<F, 3>>;

  unsigned char buf[S::sizeOf(prod)];
  S::write(prod, buf);

  math::Array<F, 3> p;

  REQUIRE(p != prod);

  S::read(p, buf);

  REQUIRE(p == prod);
}
