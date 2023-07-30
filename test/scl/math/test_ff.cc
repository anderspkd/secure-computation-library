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
#include <stdexcept>

#include "fields.h"
#include "scl/util/prg.h"

using namespace scl;

namespace {

template <typename T>
T RandomNonZero(util::PRG& prg) {
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
test::GF7 RandomNonZero<test::GF7>(util::PRG& prg) {
  auto a = test::GF7::Random(prg);
  if (a != test::GF7(6)) {
    return a + test::GF7(1);
  }
  return a;
}

}  // namespace

#ifndef SCL_FF_TEST_REPETITIONS
#define SCL_FF_TEST_REPETITIONS 50
#endif

#define REPEAT for (std::size_t i = 0; i < SCL_FF_TEST_REPETITIONS; ++i)

TEMPLATE_TEST_CASE("FF Random", "[math][ff]", FIELD_DEFS) {
  using FF = TestType;

  auto prg = util::PRG::Create();
  auto zero = FF::Zero();

  auto nz = RandomNonZero<FF>(prg);
  REQUIRE(nz != zero);
}

TEMPLATE_TEST_CASE("FF Addition", "[math][ff]", FIELD_DEFS) {
  using FF = TestType;

  auto zero = FF::Zero();
  auto prg = util::PRG::Create("FF addition");
  REPEAT {
    auto x = RandomNonZero<FF>(prg);
    auto y = RandomNonZero<FF>(prg);
    auto c = x + y;
    REQUIRE(c != x);
    REQUIRE(c != y);
    REQUIRE(c == y + x);
    x += y;
    REQUIRE(c == x);
    REQUIRE(c + zero == c);
  }
}

TEMPLATE_TEST_CASE("FF Negation", "[math][ff]", FIELD_DEFS) {
  using FF = TestType;

  auto zero = FF::Zero();
  REQUIRE(zero == -zero);

  auto prg = util::PRG::Create("FF negation");
  REPEAT {
    auto a = RandomNonZero<FF>(prg);
    auto a_negated = a.Negated();
    REQUIRE(a != a_negated);
    REQUIRE(a + a_negated == zero);
    REQUIRE(a_negated == -a);
    a.Negate();
    REQUIRE(a == a_negated);
    REQUIRE(a - zero == a);
  }
}

TEMPLATE_TEST_CASE("FF Subtraction", "[math][ff]", FIELD_DEFS) {
  using FF = TestType;

  auto zero = FF::Zero();
  auto prg = util::PRG::Create("FF subtraction");
  REPEAT {
    auto a = RandomNonZero<FF>(prg);
    auto b = RandomNonZero<FF>(prg);
    REQUIRE(a - b == -(b - a));
    REQUIRE(a - b == -b + a);
    REQUIRE(a - a == zero);
    auto c = a - b;
    a -= b;
    REQUIRE(c == a);
  }
}

TEMPLATE_TEST_CASE("FF Multiplication", "[math][ff]", FIELD_DEFS) {
  using FF = TestType;

  auto zero = FF::Zero();
  auto prg = util::PRG::Create("FF multiplication");
  REPEAT {
    auto a = RandomNonZero<FF>(prg);
    auto b = RandomNonZero<FF>(prg);
    REQUIRE(a * b != zero);
    REQUIRE(a * b == b * a);
    auto c = RandomNonZero<FF>(prg);
    REQUIRE(c * (a + b) == c * a + c * b);
    auto d = a * b;
    a *= b;
    REQUIRE(a == d);

    REQUIRE(a * zero == zero);
  }
}

TEMPLATE_TEST_CASE("FF Inversion", "[math][ff]", FIELD_DEFS) {
  using FF = TestType;

  auto zero = FF::Zero();
  REQUIRE_THROWS_MATCHES(
      zero.Inverse(),
      std::logic_error,
      Catch::Matchers::Message("0 not invertible modulo prime"));

  auto prg = util::PRG::Create("FF inversion");
  REPEAT {
    auto a = RandomNonZero<FF>(prg);
    auto a_inverse = a.Inverse();
    REQUIRE(a * a_inverse == FF::One());
    a.Invert();
    REQUIRE(a == a_inverse);
  }
}

TEMPLATE_TEST_CASE("FF Division", "[math][ff]", FIELD_DEFS) {
  using FF = TestType;

  auto zero = FF::Zero();
  auto prg = util::PRG::Create("FF division");
  REPEAT {
    auto a = RandomNonZero<FF>(prg);
    auto b = RandomNonZero<FF>(prg);
    REQUIRE(a / a == FF::One());
    REQUIRE(a / b == (b / a).Inverse());
    auto c = a / b;
    a /= b;
    REQUIRE(c == a);
    REQUIRE(zero / c == zero);
  }
}

TEMPLATE_TEST_CASE("FF serialization", "[math][ff]", FIELD_DEFS) {
  using FF = TestType;

  auto prg = util::PRG::Create("FF serialization");
  REPEAT {
    auto a = RandomNonZero<FF>(prg);
    unsigned char buf[FF::ByteSize()] = {0};
    a.Write(buf);

    auto b = FF::Read(buf);
    REQUIRE(a == b);
  }
}

TEMPLATE_TEST_CASE("FF Exp", "[math][ff]", FIELD_DEFS) {
  using FF = TestType;

  auto prg = util::PRG::Create("FF exp");

  auto a = RandomNonZero<FF>(prg);

  REQUIRE(a == Exp(a, 1));
  REQUIRE(a * a == Exp(a, 2));

  REQUIRE(a * a * a * a * a * a == Exp(a, 6));

  REQUIRE(FF::One() == Exp(a, 0));
}
