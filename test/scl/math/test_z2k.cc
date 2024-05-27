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
#include <sstream>

#include "scl/math/z2k.h"
#include "scl/util/prg.h"

using namespace scl;

// use sizes that ensure masking is needed
using Small = math::Z2k<62>;
using Big = math::Z2k<123>;

#define RING_DEFS Big, Small

TEMPLATE_TEST_CASE("Z2k name", "[math][ring]", RING_DEFS) {
  using Ring = TestType;

  REQUIRE(Ring::name() == std::string("Z2k"));
}

TEST_CASE("Z2k big size", "[math][ring]") {
  REQUIRE(Big::bitSize() == 123);
  REQUIRE(Big::byteSize() == 16);
}

TEST_CASE("Z2k small size", "[math][ring]") {
  REQUIRE(Small::bitSize() == 62);
  REQUIRE(Small::byteSize() == 8);
}

TEMPLATE_TEST_CASE("Z2k addition", "[math][ring]", RING_DEFS) {
  using Ring = TestType;
  auto prg = util::PRG::create("Z2k addition");

  auto a = Ring::random(prg);
  auto b = Ring::random(prg);
  auto c = a + b;
  REQUIRE(c != a);
  REQUIRE(c != b);
  REQUIRE(c == b + a);
  a += b;
  REQUIRE(c == a);
  REQUIRE(a + Ring::zero() == a);
}

TEMPLATE_TEST_CASE("Z2k negation", "[math][ring]", RING_DEFS) {
  using Ring = TestType;
  auto prg = util::PRG::create("Z2k negation");

  auto a = Ring::random(prg);
  auto a_negated = a.negated();
  REQUIRE(a != a_negated);
  REQUIRE(a + a_negated == Ring::zero());
  REQUIRE(a_negated == -a);
  a.negate();
  REQUIRE(a == a_negated);
}

TEMPLATE_TEST_CASE("Z2k subtraction", "[math][ring]", RING_DEFS) {
  using Ring = TestType;
  auto prg = util::PRG::create("Z2k subtraction");

  auto a = Ring::random(prg);
  auto b = Ring::random(prg);
  auto c = a - b;
  REQUIRE(c == -(b - a));
  a -= b;
  REQUIRE(c == a);
  REQUIRE(c - c == Ring::zero());
  REQUIRE(c - Ring::zero() == c);
}

TEMPLATE_TEST_CASE("Z2k multiplication", "[math][ring]", RING_DEFS) {
  using Ring = TestType;
  auto prg = util::PRG::create("Z2k multiplication");

  auto a = Ring::random(prg);
  auto b = Ring::random(prg);
  REQUIRE(a * b == b * a);
  auto c = Ring::random(prg);
  REQUIRE(c * (a + b) == c * a + c * b);
  auto d = a * b;
  a *= b;
  REQUIRE(a == d);
  REQUIRE(a * Ring::one() == a);
}

namespace {

template <typename RING>
RING randomInvertible(util::PRG& prg) {
  RING z;
  while (z.lsb() == 0) {
    z = RING::random(prg);
  }
  return z;
}
}  // namespace

TEMPLATE_TEST_CASE("Z2k inverses", "[math][ring]", RING_DEFS) {
  using Ring = TestType;
  auto prg = util::PRG::create("Z2k inverses");

  auto a = randomInvertible<Ring>(prg);
  auto a_inverse = a.inverse();
  REQUIRE(a * a_inverse == TestType(1));
  REQUIRE_THROWS_MATCHES(
      Ring::zero().inverse(),
      std::logic_error,
      Catch::Matchers::Message("value not invertible modulo 2^K"));
}

TEMPLATE_TEST_CASE("Z2k division", "[math][ring]", RING_DEFS) {
  using Ring = TestType;
  auto prg = util::PRG::create("Z2k division");

  auto a = randomInvertible<Ring>(prg);
  auto b = randomInvertible<Ring>(prg);

  REQUIRE(a / a == TestType(1));
  REQUIRE(a / b == (b / a).inverse());
  auto c = a / b;
  a /= b;
  REQUIRE(c == a);
}

TEMPLATE_TEST_CASE("Z2k serialization", "[math][ring]", RING_DEFS) {
  using Ring = TestType;
  auto prg = util::PRG::create("Z2k serialization");

  auto a = Ring::random(prg);
  unsigned char buffer[TestType::byteSize()];
  a.write(buffer);
  auto b = Ring::read(buffer);
  REQUIRE(a == b);
}

TEMPLATE_TEST_CASE("Z2k to string", "[math][ring]", RING_DEFS) {
  using Ring = TestType;

  Ring x(0x7b);
  REQUIRE(x.toString() == "7b");
  std::stringstream ss;
  ss << x;
  REQUIRE(ss.str() == "7b");
}

TEST_CASE("Z2k truncation", "[math]") {
  using Z2k = math::Z2k<32>;

  Z2k a(0x34abcdef11);
  Z2k b(0x00abcdef11);

  REQUIRE(a == b);

  unsigned char buffer_a[Z2k::byteSize() + 2] = {0};
  unsigned char buffer_b[Z2k::byteSize() + 2] = {0};
  buffer_a[4] = 0xff;
  buffer_a[5] = 0xff;
  buffer_b[4] = 0xff;
  buffer_b[5] = 0xff;
  a.write(buffer_a);
  b.write(buffer_b);

  REQUIRE(buffer_a[0] == buffer_b[0]);
  REQUIRE(buffer_a[1] == buffer_b[1]);
  REQUIRE(buffer_a[2] == buffer_b[2]);
  REQUIRE(buffer_a[3] == buffer_b[3]);
  // excess is not written to
  REQUIRE(buffer_a[4] == 0xff);
  REQUIRE(buffer_a[5] == 0xff);
  REQUIRE(buffer_b[4] == 0xff);
  REQUIRE(buffer_b[5] == 0xff);

  REQUIRE(a.toString() == "abcdef11");
  REQUIRE(b.toString() == "abcdef11");

  std::stringstream ss_a;
  std::stringstream ss_b;
  ss_a << a;
  ss_b << b;
  REQUIRE(ss_a.str() == ss_b.str());

  Z2k c = Z2k::fromString("34abcdef11");
  REQUIRE(c == a);
  REQUIRE(c == b);
}
