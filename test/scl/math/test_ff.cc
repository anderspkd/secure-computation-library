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
#include "scl/math/curves/secp256k1.h"
#include "scl/math/fp.h"
#include "scl/prg.h"

using Mersenne61 = scl::Fp<61>;
using Mersenne127 = scl::Fp<127>;
using GF7 = scl::FF<scl::details::GF7>;

#ifdef SCL_ENABLE_EC_TESTS
using Secp256k1_Field = scl::FF<scl::details::Secp256k1::Field>;
using Secp256k1_Order = scl::FF<scl::details::Secp256k1::Order>;
#endif

template <typename T>
T RandomNonZero(scl::PRG& prg) {
  auto a = T::Random(prg);
  for (std::size_t i = 0; i < 10; ++i) {
    if (a == T::Zero()) {
      a = T::Random(prg);
    }
    break;
  }
  if (a == T::Zero()) {
    throw std::logic_error("could not generate a non-zero random value");
  }
  return a;
}

// Specialization for the very small field since it's apparently possible to hit
// zero 10 times in a row...
template <>
GF7 RandomNonZero<GF7>(scl::PRG& prg) {
  auto a = GF7::Random(prg);
  if (a != GF7(6)) {
    return a + GF7(1);
  }
  return a;
}

#define REPEAT for (std::size_t i = 0; i < 50; ++i)

#ifdef SCL_ENABLE_EC_TESTS
#define ARG_LIST Mersenne61, Mersenne127, GF7, Secp256k1_Field, Secp256k1_Order
#else
#define ARG_LIST Mersenne61, Mersenne127, GF7
#endif

TEMPLATE_TEST_CASE("FF", "[math]", ARG_LIST) {
  scl::PRG prg;
  auto zero = TestType();

  SECTION("random") {
    auto a = RandomNonZero<TestType>(prg);
    REQUIRE(a != zero);
  }

  SECTION("addition") {
    auto d = TestType(-1);
    auto e = TestType(1);
    REQUIRE(d + e == TestType(0));

    REPEAT {
      auto a = RandomNonZero<TestType>(prg);
      auto b = RandomNonZero<TestType>(prg);
      auto c = a + b;
      REQUIRE(c != a);
      REQUIRE(c != b);
      REQUIRE(c == b + a);
      a += b;
      REQUIRE(c == a);
      REQUIRE(c + zero == c);
    }
  }

  SECTION("negation") {
    REPEAT {
      auto a = RandomNonZero<TestType>(prg);
      auto a_negated = a.Negated();
      REQUIRE(a != a_negated);
      REQUIRE(a + a_negated == zero);
      REQUIRE(a_negated == -a);
      a.Negate();
      REQUIRE(a == a_negated);
      REQUIRE(a - zero == a);
    }
  }

  SECTION("subtraction") {
    REPEAT {
      auto a = RandomNonZero<TestType>(prg);
      auto b = RandomNonZero<TestType>(prg);
      REQUIRE(a - b == -(b - a));
      REQUIRE(a - b == -b + a);
      auto c = a - b;
      a -= b;
      REQUIRE(c == a);
      REQUIRE(c * zero == zero);
    }
  }

  SECTION("multiplication") {
    REPEAT {
      auto a = RandomNonZero<TestType>(prg);
      auto b = RandomNonZero<TestType>(prg);
      REQUIRE(a * b == b * a);
      auto c = RandomNonZero<TestType>(prg);
      REQUIRE(c * (a + b) == c * a + c * b);
      auto d = a * b;
      a *= b;
      REQUIRE(a == d);
    }
  }

  SECTION("inverses") {
    REQUIRE_THROWS_MATCHES(
        zero.Inverse(), std::logic_error,
        Catch::Matchers::Message("0 not invertible modulo prime"));

    REPEAT {
      auto a = RandomNonZero<TestType>(prg);
      auto a_inverse = a.Inverse();
      REQUIRE(a * a_inverse == TestType(1));
      a.Invert();
      REQUIRE(a == a_inverse);
    }
  }

  SECTION("divide") {
    REPEAT {
      auto a = RandomNonZero<TestType>(prg);
      auto b = RandomNonZero<TestType>(prg);
      REQUIRE(a / a == TestType(1));
      REQUIRE(a / b == (b / a).Inverse());
      auto c = a / b;
      a /= b;
      REQUIRE(c == a);
      REQUIRE(zero / c == zero);
    }
  }
}
