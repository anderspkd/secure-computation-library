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

  REQUIRE(Ring::Name() == std::string("Z2k"));
}

TEST_CASE("Z2k big size", "[math][ring]") {
  REQUIRE(Big::BitSize() == 123);
  REQUIRE(Big::ByteSize() == 16);
}

TEST_CASE("Z2k small size", "[math][ring]") {
  REQUIRE(Small::BitSize() == 62);
  REQUIRE(Small::ByteSize() == 8);
}

TEMPLATE_TEST_CASE("Z2k addition", "[math][ring]", RING_DEFS) {
  using Ring = TestType;
  auto prg = util::PRG::Create("Z2k addition");

  auto a = Ring::Random(prg);
  auto b = Ring::Random(prg);
  auto c = a + b;
  REQUIRE(c != a);
  REQUIRE(c != b);
  REQUIRE(c == b + a);
  a += b;
  REQUIRE(c == a);
  REQUIRE(a + Ring::Zero() == a);
}

TEMPLATE_TEST_CASE("Z2k negation", "[math][ring]", RING_DEFS) {
  using Ring = TestType;
  auto prg = util::PRG::Create("Z2k negation");

  auto a = Ring::Random(prg);
  auto a_negated = a.Negated();
  REQUIRE(a != a_negated);
  REQUIRE(a + a_negated == Ring::Zero());
  REQUIRE(a_negated == -a);
  a.Negate();
  REQUIRE(a == a_negated);
}

TEMPLATE_TEST_CASE("Z2k subtraction", "[math][ring]", RING_DEFS) {
  using Ring = TestType;
  auto prg = util::PRG::Create("Z2k subtraction");

  auto a = Ring::Random(prg);
  auto b = Ring::Random(prg);
  auto c = a - b;
  REQUIRE(c == -(b - a));
  a -= b;
  REQUIRE(c == a);
  REQUIRE(c - c == Ring::Zero());
  REQUIRE(c - Ring::Zero() == c);
}

TEMPLATE_TEST_CASE("Z2k multiplication", "[math][ring]", RING_DEFS) {
  using Ring = TestType;
  auto prg = util::PRG::Create("Z2k multiplication");

  auto a = Ring::Random(prg);
  auto b = Ring::Random(prg);
  REQUIRE(a * b == b * a);
  auto c = Ring::Random(prg);
  REQUIRE(c * (a + b) == c * a + c * b);
  auto d = a * b;
  a *= b;
  REQUIRE(a == d);
  REQUIRE(a * Ring::One() == a);
}

namespace {

template <typename T>
T RandomInvertible(util::PRG& prg) {
  T z;
  while (z.Lsb() == 0) {
    z = T::Random(prg);
  }
  return z;
}
}  // namespace

TEMPLATE_TEST_CASE("Z2k inverses", "[math][ring]", RING_DEFS) {
  using Ring = TestType;
  auto prg = util::PRG::Create("Z2k inverses");

  auto a = RandomInvertible<Ring>(prg);
  auto a_inverse = a.Inverse();
  REQUIRE(a * a_inverse == TestType(1));
  REQUIRE_THROWS_MATCHES(
      Ring::Zero().Inverse(),
      std::logic_error,
      Catch::Matchers::Message("value not invertible modulo 2^K"));
}

TEMPLATE_TEST_CASE("Z2k division", "[math][ring]", RING_DEFS) {
  using Ring = TestType;
  auto prg = util::PRG::Create("Z2k division");

  auto a = RandomInvertible<Ring>(prg);
  auto b = RandomInvertible<Ring>(prg);

  REQUIRE(a / a == TestType(1));
  REQUIRE(a / b == (b / a).Inverse());
  auto c = a / b;
  a /= b;
  REQUIRE(c == a);
}

TEMPLATE_TEST_CASE("Z2k serialization", "[math][ring]", RING_DEFS) {
  using Ring = TestType;
  auto prg = util::PRG::Create("Z2k serialization");

  auto a = Ring::Random(prg);
  unsigned char buffer[TestType::ByteSize()];
  a.Write(buffer);
  auto b = Ring::Read(buffer);
  REQUIRE(a == b);
}

TEMPLATE_TEST_CASE("Z2k to string", "[math][ring]", RING_DEFS) {
  using Ring = TestType;

  Ring x(0x7b);
  REQUIRE(x.ToString() == "7b");
  std::stringstream ss;
  ss << x;
  REQUIRE(ss.str() == "7b");
}

TEST_CASE("Z2k truncation", "[math]") {
  using Z2k = math::Z2k<32>;

  Z2k a(0x34abcdef11);
  Z2k b(0x00abcdef11);

  REQUIRE(a == b);

  unsigned char buffer_a[Z2k::ByteSize() + 2] = {0};
  unsigned char buffer_b[Z2k::ByteSize() + 2] = {0};
  buffer_a[4] = 0xff;
  buffer_a[5] = 0xff;
  buffer_b[4] = 0xff;
  buffer_b[5] = 0xff;
  a.Write(buffer_a);
  b.Write(buffer_b);

  REQUIRE(buffer_a[0] == buffer_b[0]);
  REQUIRE(buffer_a[1] == buffer_b[1]);
  REQUIRE(buffer_a[2] == buffer_b[2]);
  REQUIRE(buffer_a[3] == buffer_b[3]);
  // excess is not written to
  REQUIRE(buffer_a[4] == 0xff);
  REQUIRE(buffer_a[5] == 0xff);
  REQUIRE(buffer_b[4] == 0xff);
  REQUIRE(buffer_b[5] == 0xff);

  REQUIRE(a.ToString() == "abcdef11");
  REQUIRE(b.ToString() == "abcdef11");

  std::stringstream ss_a;
  std::stringstream ss_b;
  ss_a << a;
  ss_b << b;
  REQUIRE(ss_a.str() == ss_b.str());

  Z2k c = Z2k::FromString("34abcdef11");
  REQUIRE(c == a);
  REQUIRE(c == b);
}
