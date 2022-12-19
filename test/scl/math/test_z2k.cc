/**
 * @file test_z2k.cc
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
#include <sstream>

#include "scl/math/z2k.h"
#include "scl/primitives/prg.h"

// use sizes that ensure masking is needed
using Z2k1 = scl::Z2k<62>;
using Z2k2 = scl::Z2k<123>;

TEMPLATE_TEST_CASE("Z2k", "[math]", Z2k1, Z2k2) {
  auto prg = scl::PRG::Create();
  auto zero = TestType();

  REQUIRE(std::string("Z2k") == TestType::Name());

  SECTION("random") {
    auto a = TestType::Random(prg);
    REQUIRE(a != zero);
  }

  SECTION("addition") {
    auto a = TestType::Random(prg);
    auto b = TestType::Random(prg);
    auto c = a + b;
    REQUIRE(c != a);
    REQUIRE(c != b);
    REQUIRE(c == b + a);
    a += b;
    REQUIRE(c == a);
  }

  SECTION("negation") {
    auto a = TestType::Random(prg);
    auto a_negated = a.Negated();
    REQUIRE(a != a_negated);
    REQUIRE(a + a_negated == zero);
    REQUIRE(a_negated == -a);
    a.Negate();
    REQUIRE(a == a_negated);
  }

  SECTION("subtraction") {
    auto a = TestType::Random(prg);
    auto b = TestType::Random(prg);
    auto c = a - b;
    REQUIRE(c == -(b - a));
    a -= b;
    REQUIRE(c == a);
  }

  SECTION("multiplication") {
    auto a = TestType::Random(prg);
    auto b = TestType::Random(prg);
    REQUIRE(a * b == b * a);
    auto c = TestType::Random(prg);
    REQUIRE(c * (a + b) == c * a + c * b);
    auto d = a * b;
    a *= b;
    REQUIRE(a == d);
  }

#define RANDOM_INVERTIBLE(var) \
  TestType var;                \
  while ((var).Lsb() == 0) (var) = TestType::Random(prg)

  SECTION("inverses") {
    RANDOM_INVERTIBLE(a);
    auto a_inverse = a.Inverse();
    REQUIRE(a * a_inverse == TestType(1));
    REQUIRE_THROWS_MATCHES(
        zero.Inverse(),
        std::logic_error,
        Catch::Matchers::Message("value not invertible modulo 2^K"));
  }

  SECTION("divide") {
    RANDOM_INVERTIBLE(a);
    RANDOM_INVERTIBLE(b);
    REQUIRE(a / a == TestType(1));
    REQUIRE(a / b == (b / a).Inverse());
    auto c = a / b;
    a /= b;
    REQUIRE(c == a);
  }

#undef RANDOM_INVERTIBLE

  SECTION("Read/Write") {
    auto a = TestType::Random(prg);
    unsigned char buffer[TestType::ByteSize()];
    a.Write(buffer);
    auto b = TestType::Read(buffer);
    REQUIRE(a == b);
  }

  SECTION("ToString") {
    TestType x(0x7b);
    REQUIRE(x.ToString() == "7b");
    std::stringstream ss;
    ss << x;
    REQUIRE(ss.str() == "7b");
  }
}

TEST_CASE("Z2k sizes", "[math]") {
  using Z2k = scl::Z2k<62>;
  REQUIRE(Z2k::SpecifiedBitSize() == 62);
  REQUIRE(Z2k::BitSize() == 62);
  REQUIRE(Z2k::ByteSize() == 8);
}

TEST_CASE("Z2k truncation", "[math]") {
  using Z2k = scl::Z2k<32>;

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
