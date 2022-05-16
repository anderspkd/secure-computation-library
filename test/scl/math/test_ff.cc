/**
 * @file test_ff.cc
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

#include "../gf7.h"
#include "scl/math/ff.h"
#include "scl/prg.h"

using Field1 = scl::FF<61>;
using Field2 = scl::FF<127>;
using Field3 = scl::details::FF<0, scl::details::GF7>;

template <typename T>
static T RandomNonZero(scl::PRG& prg) {
  auto a = T::Random(prg);
  for (std::size_t i = 0; i < 10; ++i) {
    if (a == T{}) a = T::Random(prg);
    break;
  }
  if (a == T{})
    throw std::logic_error("could not generate a non-zero random value");
  return a;
}

TEMPLATE_TEST_CASE("FF", "[math]", Field1, Field2, Field3) {
  scl::PRG prg;
  auto zero = TestType();

  SECTION("random") {
    auto a = RandomNonZero<TestType>(prg);
    REQUIRE(a != zero);
  }

  SECTION("addition") {
    auto a = RandomNonZero<TestType>(prg);
    auto b = RandomNonZero<TestType>(prg);
    auto c = a + b;
    REQUIRE(c != a);
    REQUIRE(c != b);
    REQUIRE(c == b + a);
    a += b;
    REQUIRE(c == a);

    auto d = TestType(-1);
    auto e = TestType(1);
    REQUIRE(d + e == TestType(0));
  }

  SECTION("negation") {
    auto a = RandomNonZero<TestType>(prg);
    auto a_negated = a.Negated();
    REQUIRE(a != a_negated);
    REQUIRE(a + a_negated == zero);
    REQUIRE(a_negated == -a);
    a.Negate();
    REQUIRE(a == a_negated);
  }

  SECTION("subtraction") {
    auto a = RandomNonZero<TestType>(prg);
    auto b = RandomNonZero<TestType>(prg);
    REQUIRE(a - b == -(b - a));
    REQUIRE(a - b == -b + a);
    auto c = a - b;
    a -= b;
    REQUIRE(c == a);
  }

  SECTION("multiplication") {
    auto a = RandomNonZero<TestType>(prg);
    auto b = RandomNonZero<TestType>(prg);
    REQUIRE(a * b == b * a);
    auto c = RandomNonZero<TestType>(prg);
    REQUIRE(c * (a + b) == c * a + c * b);
    auto d = a * b;
    a *= b;
    REQUIRE(a == d);
  }

  SECTION("inverses") {
    auto a = RandomNonZero<TestType>(prg);
    auto a_inverse = a.Inverse();
    REQUIRE(a * a_inverse == TestType(1));
    REQUIRE_THROWS_MATCHES(
        zero.Inverse(), std::logic_error,
        Catch::Matchers::Message("0 not invertible modulo prime"));
    a.Invert();
    REQUIRE(a == a_inverse);
  }

  SECTION("divide") {
    auto a = RandomNonZero<TestType>(prg);
    auto b = RandomNonZero<TestType>(prg);
    REQUIRE(a / a == TestType(1));
    REQUIRE(a / b == (b / a).Inverse());
    auto c = a / b;
    a /= b;
    REQUIRE(c == a);
  }
}
