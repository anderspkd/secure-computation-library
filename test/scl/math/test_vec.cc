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

#include <algorithm>
#include <catch2/catch.hpp>
#include <cstdint>
#include <sstream>
#include <stdexcept>

#include "scl/math/curves/secp256k1.h"
#include "scl/math/ec.h"
#include "scl/math/fp.h"
#include "scl/math/mat.h"
#include "scl/math/vec.h"
#include "scl/util/traits.h"

using namespace scl;

using FF = math::Fp<61>;
using Vec = math::Vec<FF>;

auto v0 = Vec{FF(1), FF(2), FF(3)};
auto v1 = Vec{FF(2), FF(123), FF(5)};

TEST_CASE("Vector access", "[math][la]") {
  REQUIRE(v0[0] == FF(1));
  REQUIRE(v1[1] == FF(123));
}

TEST_CASE("Vector mutate", "[math][la]") {
  auto v3 = v0;
  v3[0] = FF(444);
  REQUIRE(v3[0] == FF(444));
  REQUIRE(v0[0] == FF(1));
}

TEST_CASE("Vector size", "[math][la]") {
  REQUIRE(v0.Size() == 3);
  REQUIRE(v1.Size() == 3);
  auto v3 = Vec(100);
  REQUIRE(v3.Size() == 100);
}

TEST_CASE("Vector addition", "[math][la]") {
  auto v2 = v0.Add(v1);
  REQUIRE(v2[0] == FF(3));
  REQUIRE(v2[1] == FF(125));
  REQUIRE(v2[2] == FF(8));
  v2.AddInPlace(v0);
  REQUIRE(v2.Equals(v0.Add(v1).Add(v0)));
}

TEST_CASE("Vector subtract", "[math][la]") {
  auto v2 = v0.Subtract(v1);
  REQUIRE(v2.Equals(Vec{FF(1) - FF(2), FF(2) - FF(123), FF(3) - FF(5)}));
  v2.SubtractInPlace(v1);
  REQUIRE(v2.Equals(v0.Subtract(v1).Subtract(v1)));
}

TEST_CASE("Vector multiply entry-wise", "[math][la]") {
  auto v2 = v0.MultiplyEntryWise(v1);
  REQUIRE(v2.Equals(Vec{FF(2), FF(246), FF(15)}));
  v2.MultiplyEntryWiseInPlace(v1);
  REQUIRE(v2.Equals(v0.MultiplyEntryWise(v1).MultiplyEntryWise(v1)));
}

TEST_CASE("Vector dot", "[math][la]") {
  auto dp = v0.Dot(v1);
  REQUIRE(dp == FF(263));
}

TEST_CASE("Vector scalar multiplication", "[math][la]") {
  auto v2 = v1.ScalarMultiply(FF(2));
  REQUIRE(v2.Equals(Vec{FF(4), FF(246), FF(10)}));
  v2.ScalarMultiplyInPlace(FF(2));
  REQUIRE(v2.Equals(Vec{FF(8), FF(492), FF(20)}));
}

TEST_CASE("Vector to matrix", "[math][la]") {
  auto m0 = v0.ToRowMatrix();
  REQUIRE(m0.Rows() == 1);
  REQUIRE(m0.Cols() == 3);
  auto m1 = v1.ToColumnMatrix();
  REQUIRE(m1.Rows() == 3);
  REQUIRE(m1.Cols() == 1);
}

TEST_CASE("Vector to string", "[math][la]") {
  REQUIRE(v0.ToString() == "[1, 2, 3]");
  REQUIRE(v1.ToString() == "[2, 7b, 5]");
  std::stringstream ss;
  ss << v0;
  REQUIRE(ss.str() == "[1, 2, 3]");
  Vec v;
  REQUIRE(v.ToString() == "[ EMPTY VECTOR ]");
}

TEST_CASE("Vector incompatible", "[math][la]") {
  auto v2 = Vec{FF(2), FF(3)};
  REQUIRE(!v2.Equals(v1));
  REQUIRE_THROWS_MATCHES(v2.Add(v1),
                         std::invalid_argument,
                         Catch::Matchers::Message("Vec sizes mismatch"));
}

TEST_CASE("Vector to std::vector", "[math][la]") {
  auto stl0 = v0.ToStlVector();
  REQUIRE(stl0 == std::vector<FF>{FF(1), FF(2), FF(3)});
}

TEST_CASE("Vector random", "[math][la]") {
  auto prg = util::PRG::Create("Vector random");
  auto r = Vec::Random(3, prg);
  auto zero = FF();
  REQUIRE(r.Size() == 3);
  REQUIRE(r[0] != zero);
  REQUIRE(r[0] != v0[0]);
  REQUIRE(r[1] != zero);
  REQUIRE(r[1] != v0[1]);
  REQUIRE(r[2] != zero);
  REQUIRE(r[2] != v0[2]);
}

TEST_CASE("Vector range", "[math][la]") {
  auto v = Vec::Range(1, 4);
  REQUIRE(v[0] == FF(1));
  REQUIRE(v[1] == FF(2));
  REQUIRE(v[2] == FF(3));

  REQUIRE(Vec::Range(1, 1) == Vec{});

  REQUIRE_THROWS_MATCHES(Vec::Range(2, 1),
                         std::invalid_argument,
                         Catch::Matchers::Message("invalid range"));
}

TEST_CASE("Vector iterator", "[math][la]") {
  auto v2 = Vec{FF(1), FF(2), FF(3)};
  std::size_t i = 0;
  for (auto& v : v0) {
    REQUIRE(v == v2[i++]);
  }

  auto count = std::count(v2.begin(), v2.end(), FF(2));
  REQUIRE(count == 1);

  auto v3 = Vec(v2.begin(), v2.end());
  REQUIRE(v3.Equals(v2));
}

TEST_CASE("Vector sub vector", "[math][la]") {
  auto v = Vec{FF(1), FF(2), FF(3), FF(4)};
  REQUIRE(v.SubVector(1, 2) == Vec{FF(2)});
  REQUIRE(v.SubVector(1, 3) == Vec{FF(2), FF(3)});
  REQUIRE(v.SubVector(1, 1) == Vec{});
  REQUIRE(v.SubVector(2) == Vec{FF(1), FF(2)});

  REQUIRE_THROWS_MATCHES(v.SubVector(2, 1),
                         std::logic_error,
                         Catch::Matchers::Message("invalid range"));
}

TEST_CASE("Vector scalar EC", "[math]") {
  using Curve = math::EC<math::Secp256k1>;

  auto v = math::Vec<Curve>{Curve::Generator(),
                            Curve::Generator(),
                            Curve::Generator()};

  const auto s = Curve::ScalarField(123);
  auto w = v.ScalarMultiply(s);

  REQUIRE(w[0] == Curve::Generator() * s);
  REQUIRE(w[1] == Curve::Generator() * s);
  REQUIRE(w[2] == Curve::Generator() * s);

  const auto z = math::Number(123);
  auto u = w.ScalarMultiply(math::Number(123));

  REQUIRE(u[0] == w[0] * z);
  REQUIRE(u[1] == w[1] * z);
  REQUIRE(u[2] == w[2] * z);
}
