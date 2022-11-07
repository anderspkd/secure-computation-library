/**
 * @file test_vec.cc
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

#include <algorithm>
#include <catch2/catch.hpp>
#include <cstdint>
#include <sstream>

#include "scl/math/fp.h"
#include "scl/math/mat.h"
#include "scl/math/vec.h"

using F = scl::Fp<61>;
using Vec = scl::Vec<F>;

TEST_CASE("Vector", "[math]") {
  auto v0 = Vec{F(1), F(2), F(3)};
  auto v1 = Vec{F(2), F(123), F(5)};

  REQUIRE(!v0.Equals(v1));

  SECTION("Access") {
    REQUIRE(v0[0] == F(1));
    REQUIRE(v1[1] == F(123));
  }

  SECTION("Mutate") {
    auto v3 = v0;
    v3[0] = F(444);
    REQUIRE(v3[0] == F(444));
    REQUIRE(v0[0] == F(1));
  }

  SECTION("Size") {
    REQUIRE(v0.Size() == 3);
    REQUIRE(v1.Size() == 3);
    auto v3 = Vec(100);
    REQUIRE(v3.Size() == 100);
  }

  SECTION("Addition") {
    auto v2 = v0.Add(v1);
    REQUIRE(v2[0] == F(3));
    REQUIRE(v2[1] == F(125));
    REQUIRE(v2[2] == F(8));
    v2.AddInPlace(v0);
    REQUIRE(v2.Equals(v0.Add(v1).Add(v0)));
  }

  SECTION("Subtract") {
    auto v2 = v0.Subtract(v1);
    REQUIRE(v2.Equals(Vec{F(1) - F(2), F(2) - F(123), F(3) - F(5)}));
    v2.SubtractInPlace(v1);
    REQUIRE(v2.Equals(v0.Subtract(v1).Subtract(v1)));
  }

  SECTION("Multiply") {
    auto v2 = v0.MultiplyEntryWise(v1);
    REQUIRE(v2.Equals(Vec{F(2), F(246), F(15)}));
    v2.MultiplyEntryWiseInPlace(v1);
    REQUIRE(v2.Equals(v0.MultiplyEntryWise(v1).MultiplyEntryWise(v1)));
  }

  SECTION("Dot") {
    auto dp = v0.Dot(v1);
    REQUIRE(dp == F(263));
  }

  SECTION("ScalarMult") {
    auto v2 = v1.ScalarMultiply(F(2));
    REQUIRE(v2.Equals(Vec{F(4), F(246), F(10)}));
    v2.ScalarMultiplyInPlace(F(2));
    REQUIRE(v2.Equals(Vec{F(8), F(492), F(20)}));
  }

  SECTION("ToMatrix") {
    auto m0 = v0.ToRowMatrix();
    REQUIRE(m0.Rows() == 1);
    REQUIRE(m0.Cols() == 3);
    auto m1 = v1.ToColumnMatrix();
    REQUIRE(m1.Rows() == 3);
    REQUIRE(m1.Cols() == 1);
  }

  SECTION("ToString") {
    REQUIRE(v0.ToString() == "[1, 2, 3]");
    REQUIRE(v1.ToString() == "[2, 7b, 5]");
    std::stringstream ss;
    ss << v0;
    REQUIRE(ss.str() == "[1, 2, 3]");
    Vec v;
    REQUIRE(v.ToString() == "[ EMPTY_VECTOR ]");
  }

  SECTION("Incompatible") {
    auto v2 = Vec{F(2), F(3)};
    REQUIRE(!v2.Equals(v1));
    REQUIRE_THROWS_MATCHES(v2.Add(v1), std::invalid_argument,
                           Catch::Matchers::Message("Vec sizes mismatch"));
  }

  SECTION("AsSTL") {
    auto stl0 = v0.ToStlVector();
    REQUIRE(stl0 == std::vector<F>{F(1), F(2), F(3)});
  }

  SECTION("Random") {
    scl::PRG prg;
    auto r = Vec::Random(3, prg);
    auto zero = F();
    REQUIRE(r.Size() == 3);
    REQUIRE(r[0] != zero);
    REQUIRE(r[0] != v0[0]);
    REQUIRE(r[1] != zero);
    REQUIRE(r[1] != v0[1]);
    REQUIRE(r[2] != zero);
    REQUIRE(r[2] != v0[2]);
  }

  SECTION("Range") {
    auto v = Vec::Range(1, 4);
    REQUIRE(v[0] == F(1));
    REQUIRE(v[1] == F(2));
    REQUIRE(v[2] == F(3));
  }

  SECTION("iterators") {
    auto v2 = Vec{F(1), F(2), F(3)};
    std::size_t i = 0;
    for (auto& v : v0) {
      REQUIRE(v == v2[i++]);
    }

    auto count = std::count(v2.begin(), v2.end(), F(2));
    REQUIRE(count == 1);

    auto v3 = Vec(v2.begin(), v2.end());
    REQUIRE(v3.Equals(v2));
  }

  SECTION("subvector") {
    auto v = Vec{F(1), F(2), F(3), F(4)};
    REQUIRE(v.SubVector(1, 2) == Vec{F(2)});
    REQUIRE(v.SubVector(1, 3) == Vec{F(2), F(3)});
    REQUIRE(v.SubVector(1, 1) == Vec{});
    REQUIRE(v.SubVector(2) == Vec{F(1), F(2)});
  }
}
