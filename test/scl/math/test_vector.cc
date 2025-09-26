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

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <sstream>
#include <stdexcept>

#include "scl/math/ec.h"
#include "scl/math/ff.h"
#include "scl/math/matrix.h"
#include "scl/math/mersenne61.h"
#include "scl/math/secp256k1.h"
#include "scl/math/vector.h"

using namespace scl;

using Elem = FF<Mersenne61>;

auto v0 = Vector{Elem(1), Elem(2), Elem(3)};
auto v1 = Vector{Elem(2), Elem(123), Elem(5)};

TEST_CASE("Vector access", "[math][la]") {
  REQUIRE(v0[0] == Elem(1));
  REQUIRE(v1[1] == Elem(123));
}

TEST_CASE("Vector mutate", "[math][la]") {
  auto v3 = v0;
  v3[0] = Elem(444);
  REQUIRE(v3[0] == Elem(444));
  REQUIRE(v0[0] == Elem(1));
}

TEST_CASE("Vector size", "[math][la]") {
  REQUIRE(v0.size() == 3);
  REQUIRE(v1.size() == 3);
  auto v3 = Vector<Elem>(100);
  REQUIRE(v3.size() == 100);
}

TEST_CASE("Vector addition", "[math][la]") {
  auto v2 = v0.add(v1);
  REQUIRE(v2[0] == Elem(3));
  REQUIRE(v2[1] == Elem(125));
  REQUIRE(v2[2] == Elem(8));
  v2.addInPlace(v0);
  REQUIRE(v2.equals(v0.add(v1).add(v0)));
}

TEST_CASE("Vector subtract", "[math][la]") {
  auto v2 = v0.subtract(v1);
  REQUIRE(v2.equals(
      Vector{Elem(1) - Elem(2), Elem(2) - Elem(123), Elem(3) - Elem(5)}));
  v2.subtractInPlace(v1);
  REQUIRE(v2.equals(v0.subtract(v1).subtract(v1)));
}

TEST_CASE("Vector multiply entry-wise", "[math][la]") {
  auto v2 = v0.multiplyEntryWise(v1);
  REQUIRE(v2.equals(Vector{Elem(2), Elem(246), Elem(15)}));
  v2.multiplyEntryWiseInPlace(v1);
  REQUIRE(v2.equals(v0.multiplyEntryWise(v1).multiplyEntryWise(v1)));
}

TEST_CASE("Vector dot", "[math][la]") {
  auto dp = v0.dot(v1);
  REQUIRE(dp == Elem(263));
}

TEST_CASE("Vector scalar multiplication", "[math][la]") {
  auto v2 = v1.scalarMultiply(Elem(2));
  REQUIRE(v2.equals(Vector{Elem(4), Elem(246), Elem(10)}));
  v2.scalarMultiplyInPlace(Elem(2));
  REQUIRE(v2.equals(Vector{Elem(8), Elem(492), Elem(20)}));
}

TEST_CASE("Vector to matrix", "[math][la]") {
  auto m0 = v0.toRowMatrix();
  REQUIRE(m0.rows() == 1);
  REQUIRE(m0.cols() == 3);
  auto m1 = v1.toColumnMatrix();
  REQUIRE(m1.rows() == 3);
  REQUIRE(m1.cols() == 1);
}

TEST_CASE("Vector to string", "[math][la]") {
  REQUIRE(v0.toString() == "[1, 2, 3]");
  REQUIRE(v1.toString() == "[2, 7b, 5]");
  std::stringstream ss;
  ss << v0;
  REQUIRE(ss.str() == "[1, 2, 3]");
  Vector<Elem> v;
  REQUIRE(v.toString() == "[ EMPTY VECTOR ]");
}

TEST_CASE("Vector incompatible", "[math][la]") {
  auto v2 = Vector{Elem(2), Elem(3)};
  REQUIRE(!v2.equals(v1));
  REQUIRE_THROWS_MATCHES(v2.add(v1),
                         std::invalid_argument,
                         Catch::Matchers::Message("Vec sizes mismatch"));
}

TEST_CASE("Vector to std::vector", "[math][la]") {
  auto stl0 = v0.toStlVector();
  REQUIRE(stl0 == std::vector<Elem>{Elem(1), Elem(2), Elem(3)});
}

TEST_CASE("Vector random", "[math][la]") {
  auto prg = PRG::create("Vector random");
  auto r = Vector<Elem>::random(3, prg);
  auto zero = Elem();
  REQUIRE(r.size() == 3);
  REQUIRE(r[0] != zero);
  REQUIRE(r[0] != v0[0]);
  REQUIRE(r[1] != zero);
  REQUIRE(r[1] != v0[1]);
  REQUIRE(r[2] != zero);
  REQUIRE(r[2] != v0[2]);
}

TEST_CASE("Vector range", "[math][la]") {
  auto v = Vector<Elem>::range(1, 4);
  REQUIRE(v[0] == Elem(1));
  REQUIRE(v[1] == Elem(2));
  REQUIRE(v[2] == Elem(3));

  REQUIRE(Vector<Elem>::range(1, 1).empty());

  REQUIRE_THROWS_MATCHES(Vector<Elem>::range(2, 1),
                         std::invalid_argument,
                         Catch::Matchers::Message("invalid range"));
}

TEST_CASE("Vector iterator", "[math][la]") {
  auto v2 = Vector{Elem(1), Elem(2), Elem(3)};
  std::size_t i = 0;
  for (auto& v : v0) {
    REQUIRE(v == v2[i++]);
  }

  auto count = std::count(v2.begin(), v2.end(), Elem(2));
  REQUIRE(count == 1);

  auto v3 = Vector<Elem>(v2.begin(), v2.end());
  REQUIRE(v3.equals(v2));
}

TEST_CASE("Vector sub vector", "[math][la]") {
  auto v = Vector{Elem(1), Elem(2), Elem(3), Elem(4)};
  REQUIRE(v.subVector(1, 2) == Vector{Elem(2)});
  REQUIRE(v.subVector(1, 3) == Vector{Elem(2), Elem(3)});
  REQUIRE(v.subVector(1, 1).empty());
  REQUIRE(v.subVector(2) == Vector{Elem(1), Elem(2)});

  REQUIRE_THROWS_MATCHES(v.subVector(2, 1),
                         std::logic_error,
                         Catch::Matchers::Message("invalid range"));
}

TEST_CASE("Vector scalar EC", "[math]") {
  using Curve = EC<Secp256k1>;

  auto v =
      Vector<Curve>{Curve::generator(), Curve::generator(), Curve::generator()};

  const auto s = Curve::ScalarField(123);
  auto w = v.scalarMultiply(s);

  REQUIRE(w[0] == Curve::generator() * s);
  REQUIRE(w[1] == Curve::generator() * s);
  REQUIRE(w[2] == Curve::generator() * s);

  const auto z = Number(123);
  auto u = w.scalarMultiply(Number(123));

  REQUIRE(u[0] == w[0] * z);
  REQUIRE(u[1] == w[1] * z);
  REQUIRE(u[2] == w[2] * z);
}

TEST_CASE("Vector byte size", "[math]") {
  auto v = Vector{Elem(1), Elem(2)};

  REQUIRE(v.byteSize() == 2 * Elem::byteSize());
}
