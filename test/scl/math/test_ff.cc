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

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <stdexcept>

#include "fields.h"
#include "scl/util/prg.h"

using namespace scl;

namespace {

template <typename T>
T randomNonZero(util::PRG& prg) {
  auto a = T::random(prg);
  for (std::size_t i = 0; i < 10; ++i) {
    if (a == T::zero()) {
      a = T::random(prg);
    }
    break;
  }
  if (a == T::zero()) {
    throw std::logic_error("could not generate a non-zero random value");
  }
  return a;
}

// Specialization for the very small field since it's apparently possible to hit
// zero 10 times in a row...
template <>
test::GF7 randomNonZero<test::GF7>(util::PRG& prg) {
  auto a = test::GF7::random(prg);
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

  auto prg = util::PRG::create();
  auto zero = FF::zero();

  auto nz = randomNonZero<FF>(prg);
  REQUIRE(nz != zero);
}

TEMPLATE_TEST_CASE("FF Addition", "[math][ff]", FIELD_DEFS) {
  using FF = TestType;

  auto zero = FF::zero();
  auto prg = util::PRG::create("FF addition");
  REPEAT {
    auto a = randomNonZero<FF>(prg);
    auto b = randomNonZero<FF>(prg);
    auto c = a + b;
    REQUIRE(c != a);
    REQUIRE(c != b);
    REQUIRE(c == b + a);
    a += b;
    REQUIRE(c == a);
    REQUIRE(c + zero == c);

    // post-increment should return old value.
    auto old_a = a++;
    REQUIRE(a == old_a + FF::one());

    // pre-increment should return new value.
    auto old_b = b;
    auto new_b = ++b;
    REQUIRE(old_b == new_b - FF::one());
  }
}

TEMPLATE_TEST_CASE("FF Negation", "[math][ff]", FIELD_DEFS) {
  using FF = TestType;

  auto zero = FF::zero();
  REQUIRE(zero == -zero);

  auto prg = util::PRG::create("FF negation");
  REPEAT {
    auto a = randomNonZero<FF>(prg);
    auto a_negated = a.negated();
    REQUIRE(a != a_negated);
    REQUIRE(a + a_negated == zero);
    REQUIRE(a_negated == -a);
    a.negate();
    REQUIRE(a == a_negated);
    REQUIRE(a - zero == a);
  }
}

TEMPLATE_TEST_CASE("FF Subtraction", "[math][ff]", FIELD_DEFS) {
  using FF = TestType;

  auto zero = FF::zero();
  auto prg = util::PRG::create("FF subtraction");
  REPEAT {
    auto a = randomNonZero<FF>(prg);
    auto b = randomNonZero<FF>(prg);
    REQUIRE(a - b == -(b - a));
    REQUIRE(a - b == -b + a);
    REQUIRE(a - a == zero);
    auto c = a - b;
    a -= b;
    REQUIRE(c == a);

    auto old_a = a--;
    REQUIRE(a == old_a - FF::one());

    auto old_b = b;
    auto new_b = --b;
    REQUIRE(old_b == new_b + FF::one());
  }
}

TEMPLATE_TEST_CASE("FF Multiplication", "[math][ff]", FIELD_DEFS) {
  using FF = TestType;

  auto zero = FF::zero();
  auto prg = util::PRG::create("FF multiplication");
  REPEAT {
    auto a = randomNonZero<FF>(prg);
    auto b = randomNonZero<FF>(prg);
    REQUIRE(a * b != zero);
    REQUIRE(a * b == b * a);
    auto c = randomNonZero<FF>(prg);
    REQUIRE(c * (a + b) == c * a + c * b);
    auto d = a * b;
    a *= b;
    REQUIRE(a == d);

    REQUIRE(a * zero == zero);
  }
}

TEMPLATE_TEST_CASE("FF Inversion", "[math][ff]", FIELD_DEFS) {
  using FF = TestType;

  auto zero = FF::zero();
  REQUIRE_THROWS_MATCHES(
      zero.inverse(),
      std::logic_error,
      Catch::Matchers::Message("0 not invertible modulo prime"));

  auto prg = util::PRG::create("FF inversion");
  REPEAT {
    auto a = randomNonZero<FF>(prg);
    auto a_inverse = a.inverse();
    REQUIRE(a * a_inverse == FF::one());
    a.invert();
    REQUIRE(a == a_inverse);
  }
}

TEMPLATE_TEST_CASE("FF Division", "[math][ff]", FIELD_DEFS) {
  using FF = TestType;

  auto zero = FF::zero();
  auto prg = util::PRG::create("FF division");
  REPEAT {
    auto a = randomNonZero<FF>(prg);
    auto b = randomNonZero<FF>(prg);
    REQUIRE(a / a == FF::one());
    REQUIRE(a / b == (b / a).inverse());
    auto c = a / b;
    a /= b;
    REQUIRE(c == a);
    REQUIRE(zero / c == zero);
  }
}

TEMPLATE_TEST_CASE("FF serialization", "[math][ff]", FIELD_DEFS) {
  using FF = TestType;

  auto prg = util::PRG::create("FF serialization");
  REPEAT {
    auto a = randomNonZero<FF>(prg);
    unsigned char buf[FF::byteSize()] = {0};
    a.write(buf);

    auto b = FF::read(buf);
    REQUIRE(a == b);
  }
}

TEMPLATE_TEST_CASE("FF Exp", "[math][ff]", FIELD_DEFS) {
  using FF = TestType;

  auto prg = util::PRG::create("FF exp");

  auto a = randomNonZero<FF>(prg);

  REQUIRE(a == exp(a, 1));
  REQUIRE(a * a == exp(a, 2));

  REQUIRE(a * a * a * a * a * a == exp(a, 6));

  REQUIRE(FF::one() == exp(a, 0));
}
