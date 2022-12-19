/**
 * @file test_number.cc
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

#include "scl/math/number.h"
#include "scl/primitives/prg.h"

#define REPEAT_I(I) for (std::size_t i = 0; i < (I); ++i)
#define REPEAT REPEAT_I(50)

TEST_CASE("Number", "[math]") {
  auto prg = scl::PRG::Create();
  SECTION("Construction") {
    scl::Number n0(27);
    REQUIRE(n0.ToString() == "Number{1b}");
    scl::Number n1(-42);
    REQUIRE(n1.ToString() == "Number{-2a}");
    scl::Number zero;
    REQUIRE(zero.ToString() == "Number{0}");
    scl::Number zero_alt(0);
    REQUIRE(zero_alt.ToString() == "Number{0}");
    scl::Number r0 = scl::Number::Random(127, prg);
    REQUIRE(r0.ToString() == "Number{-27a8004ea0c9708441893d2808ca9457}");
    // the above is only 126 bits, but it's close enough
    REQUIRE(r0.BitSize() == 126);
    scl::Number r1 = scl::Number::Random(65, prg);
    REQUIRE(r1.ToString() == "Number{10584d2a1c30fa50d}");
    REQUIRE(r1.BitSize() == 65);

    std::stringstream ss;
    ss << r1;
    REQUIRE(ss.str() == r1.ToString());
  }

  SECTION("FromString") {
    auto x = scl::Number::FromString("7b");
    REQUIRE(x == scl::Number(0x7b));
  }

  SECTION("Assignment") {
    auto x = scl::Number::Random(100, prg);
    auto y = scl::Number::Random(100, prg);
    REQUIRE(x != y);

    scl::Number t;
    REQUIRE(t != x);
    REQUIRE(t != y);

    t = x;
    REQUIRE(t == x);
    t = y;
    REQUIRE(t == y);
  }

  SECTION("Comparison") {
    scl::Number np(1);
    scl::Number nn(-1);

    REQUIRE(np != nn);
    REQUIRE(np > nn);
    REQUIRE(np >= nn);
    REQUIRE(nn < np);
    REQUIRE(nn <= np);
    REQUIRE(nn == scl::Number(-1));
  }

  scl::Number zero(0);
  scl::Number one(1);

  SECTION("Addition") {
    scl::Number a(55);
    scl::Number b(32);

    REQUIRE(a + b == scl::Number(55 + 32));

    REPEAT {
      auto x = scl::Number::Random(100, prg);
      auto y = scl::Number::Random(100, prg);
      auto z = x + y;

      REQUIRE(z != x);
      REQUIRE(z != y);
      REQUIRE(z == x + y);
      x += y;
      REQUIRE(z == x);
      REQUIRE(z != y);
      REQUIRE(z + zero == z);
    }
  }

  SECTION("Subtraction") {
    scl::Number a(123);
    scl::Number b(555);

    REQUIRE(a - b == scl::Number(123 - 555));

    REPEAT {
      auto x = scl::Number::Random(100, prg);
      auto y = scl::Number::Random(100, prg);
      auto z = x - y;

      REQUIRE(z != x);
      REQUIRE(z != y);
      REQUIRE(z == x - y);
      REQUIRE(-z == y - x);
      x -= y;
      REQUIRE(z == x);
      REQUIRE(z != y);
      REQUIRE(z - zero == z);
    }
  }

  SECTION("Negation") {
    REQUIRE(-scl::Number(1234) == scl::Number(-1234));
  }

  SECTION("Multiplication") {
    scl::Number a(444);
    scl::Number b(312);

    REQUIRE(a * b == scl::Number(444 * 312));

    REPEAT {
      auto x = scl::Number::Random(100, prg);
      auto y = scl::Number::Random(100, prg);
      auto z = x * y;

      REQUIRE(z != x);
      REQUIRE(z != y);
      REQUIRE(z == y * x);

      auto w = scl::Number::Random(100, prg);
      REQUIRE(w * (x + y) == w * x + w * y);

      x *= y;
      REQUIRE(z == x);
      REQUIRE(z != y);
      REQUIRE(z * one == z);

      REQUIRE(z * scl::Number(-1) == -z);
    }
  }

  SECTION("Division") {
    scl::Number a(123);
    scl::Number b(43);
    REQUIRE(a / b == scl::Number(123 / 43));

    REPEAT {
      auto x = scl::Number::Random(100, prg);
      auto y = scl::Number::Random(85, prg);
      auto z = x / y;

      REQUIRE(z != x);
      REQUIRE(z != y);
      REQUIRE(one / z == y / x);

      x /= y;
      REQUIRE(z != y);
      REQUIRE(z == x);
    }
  }

  SECTION("Bitshift") {
    scl::Number a(44334);
    REQUIRE(a << 5 == scl::Number(1418688));
    REQUIRE(a >> 5 == scl::Number(1385));

    REQUIRE(a >> -5 == a << 5);
    REQUIRE(a << -5 == a >> 5);

    a <<= 5;
    REQUIRE(a == scl::Number(1418688));
    a >>= 5;
    REQUIRE(a == scl::Number(44334));
    a >>= 5;
    REQUIRE(a == scl::Number(1385));
  }

  SECTION("XOR") {
    scl::Number a(2231);
    scl::Number b(5545);
    REQUIRE((a ^ b) == scl::Number(2231 ^ 5545));

    REPEAT {
      auto x = scl::Number::Random(100, prg);
      auto y = scl::Number::Random(100, prg);
      auto z = x ^ y;

      REQUIRE(z != x);
      REQUIRE(z != y);
      REQUIRE(z == (y ^ x));

      x ^= y;
      REQUIRE(z != y);
      REQUIRE(z == x);
    }
  }

  SECTION("OR") {
    scl::Number a(2231);
    scl::Number b(5545);
    REQUIRE((a | b) == scl::Number(2231 | 5545));

    REPEAT {
      auto x = scl::Number::Random(100, prg);
      auto y = scl::Number::Random(100, prg);
      auto z = x | y;

      REQUIRE(z != x);
      REQUIRE(z != y);
      REQUIRE(z == (y | x));

      x |= y;
      REQUIRE(z != y);
      REQUIRE(z == x);
    }
  }

  SECTION("AND") {
    scl::Number a(2231);
    scl::Number b(5545);
    REQUIRE((a & b) == scl::Number(2231 & 5545));

    REPEAT {
      auto x = scl::Number::Random(100, prg);
      auto y = scl::Number::Random(100, prg);
      auto z = x & y;

      REQUIRE(z != x);
      REQUIRE(z != y);
      REQUIRE(z == (y & x));

      x &= y;
      REQUIRE(z != y);
      REQUIRE(z == x);
    }
  }

  SECTION("COM") {
    scl::Number a(55452);
    REQUIRE(~a == scl::Number(~55452));
  }

  SECTION("Bits") {
    scl::Number a(49);
    // out-of-range returns false
    REQUIRE(!a.TestBit(100));

    REQUIRE(a.TestBit(0));
    REQUIRE(!a.TestBit(1));
    REQUIRE(!a.TestBit(2));
    REQUIRE(!a.TestBit(3));
    REQUIRE(a.TestBit(4));
    REQUIRE(a.TestBit(5));
  }
}
