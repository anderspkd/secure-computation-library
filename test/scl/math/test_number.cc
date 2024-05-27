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

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <sstream>
#include <stdexcept>

#include "scl/math/number.h"
#include "scl/util/prg.h"

#ifndef SCl_NUMBER_TEST_REPETITIONS
#define SCl_NUMBER_TEST_REPETITIONS 50
#endif

using namespace scl;

using Number = math::Number;

#define REPEAT for (std::size_t i = 0; i < SCl_NUMBER_TEST_REPETITIONS; ++i)

TEST_CASE("Number create", "[math]") {
  auto prg = util::PRG::create();
  Number n0(27);
  REQUIRE(n0.toString() == "Number{1b}");
  Number n1(-42);
  REQUIRE(n1.toString() == "Number{-2a}");
  Number zero;
  REQUIRE(zero.toString() == "Number{0}");
  Number zero_alt(0);
  REQUIRE(zero_alt.toString() == "Number{0}");
  Number r0 = Number::random(127, prg);
  REQUIRE(r0.toString() == "Number{-27a8004ea0c9708441893d2808ca9457}");
  // the above is only 126 bits, but it's close enough
  REQUIRE(r0.bitSize() == 126);
  Number r1 = Number::random(65, prg);
  REQUIRE(r1.toString() == "Number{10584d2a1c30fa50d}");
  REQUIRE(r1.bitSize() == 65);

  Number p = Number::randomPrime(10, prg);
  REQUIRE(p.toString() == "Number{133}");  // 307

  std::stringstream ss;
  ss << r1;
  REQUIRE(ss.str() == r1.toString());

  Number r2(std::move(n0));
  REQUIRE(r2 == Number(27));
}

TEST_CASE("Number from string", "[math]") {
  auto x = Number::fromString("7b");
  REQUIRE(x == Number(0x7b));
}

TEST_CASE("Number assignment", "[math]") {
  auto prg = util::PRG::create("Number assignment");
  auto x = Number::random(100, prg);
  auto y = Number::random(100, prg);
  REQUIRE(x != y);

  Number t;
  REQUIRE(t != x);
  REQUIRE(t != y);

  t = x;
  REQUIRE(t == x);
  t = y;
  REQUIRE(t == y);
}

TEST_CASE("Number comparison", "[math]") {
  Number np(1);
  Number nn(-1);

  REQUIRE(np != nn);
  REQUIRE(np > nn);
  REQUIRE(np >= nn);
  REQUIRE(nn < np);
  REQUIRE(nn <= np);
  REQUIRE(nn == Number(-1));
}

TEST_CASE("Number addition", "[math]") {
  Number a(55);
  Number b(32);

  REQUIRE(a + b == Number(55 + 32));

  Number zero(0);
  auto prg = util::PRG::create("Number addition");
  REPEAT {
    auto x = Number::random(100, prg);
    auto y = Number::random(100, prg);
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

TEST_CASE("Number subtraction", "[math]") {
  Number a(123);
  Number b(555);

  REQUIRE(a - b == Number(123 - 555));

  Number zero;
  auto prg = util::PRG::create("Number subtraction");
  REPEAT {
    auto x = Number::random(100, prg);
    auto y = Number::random(100, prg);
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

TEST_CASE("Number negation", "[math]") {
  REQUIRE(-Number(1234) == Number(-1234));
  REQUIRE(-Number() == Number());
}

TEST_CASE("Number multiplication", "[math]") {
  Number a(444);
  Number b(312);

  REQUIRE(a * b == Number(444 * 312));

  Number one(1);
  Number zero;
  auto prg = util::PRG::create("Number multiplication");
  REPEAT {
    auto x = Number::random(100, prg);
    auto y = Number::random(100, prg);
    auto z = x * y;

    REQUIRE(z != x);
    REQUIRE(z != y);
    REQUIRE(z == y * x);

    auto w = Number::random(100, prg);
    REQUIRE(w * (x + y) == w * x + w * y);

    x *= y;
    REQUIRE(z == x);
    REQUIRE(z != y);
    REQUIRE(z * one == z);

    REQUIRE(z * Number(-1) == -z);

    REQUIRE(z * zero == zero);
  }
}

TEST_CASE("Number division", "[math]") {
  Number a(123);
  Number b(43);
  REQUIRE(a / b == Number(123 / 43));

  Number one(1);
  auto prg = util::PRG::create("Number division");
  REPEAT {
    auto x = Number::random(100, prg);
    auto y = Number::random(85, prg);
    auto z = x / y;

    REQUIRE(z != x);
    REQUIRE(z != y);
    REQUIRE(one / z == y / x);

    x /= y;
    REQUIRE(z != y);
    REQUIRE(z == x);
  }

  Number zero;
  REQUIRE_THROWS_MATCHES(a / zero,
                         std::logic_error,
                         Catch::Matchers::Message("division by 0"));
}

TEST_CASE("Number modulus", "[math]") {
  Number a(42);
  Number b(10);

  REQUIRE(a % b == Number(2));
  REQUIRE(b % a == Number(10));
}

TEST_CASE("Number bit-shift", "[math]") {
  Number a(44334);
  REQUIRE(a << 5 == Number(1418688));
  REQUIRE(a >> 5 == Number(1385));

  REQUIRE(a >> -5 == a << 5);
  REQUIRE(a << -5 == a >> 5);

  a <<= 5;
  REQUIRE(a == Number(1418688));
  a >>= 5;
  REQUIRE(a == Number(44334));
  a >>= 5;
  REQUIRE(a == Number(1385));
}

TEST_CASE("Number xor", "[math]") {
  Number a(2231);
  Number b(5545);
  REQUIRE((a ^ b) == Number(2231 ^ 5545));

  auto prg = util::PRG::create("Number xor");
  REPEAT {
    auto x = Number::random(100, prg);
    auto y = Number::random(100, prg);
    auto z = x ^ y;

    REQUIRE(z != x);
    REQUIRE(z != y);
    REQUIRE(z == (y ^ x));

    x ^= y;
    REQUIRE(z != y);
    REQUIRE(z == x);
  }
}

TEST_CASE("Number or", "[math]") {
  Number a(2231);
  Number b(5545);
  REQUIRE((a | b) == Number(2231 | 5545));

  auto prg = util::PRG::create("Number or");
  REPEAT {
    auto x = Number::random(100, prg);
    auto y = Number::random(100, prg);
    auto z = x | y;

    REQUIRE(z != x);
    REQUIRE(z != y);
    REQUIRE(z == (y | x));

    x |= y;
    REQUIRE(z != y);
    REQUIRE(z == x);
  }
}

TEST_CASE("Number and", "[math]") {
  Number a(2231);
  Number b(5545);
  REQUIRE((a & b) == Number(2231 & 5545));

  auto prg = util::PRG::create("Number and");
  REPEAT {
    auto x = Number::random(100, prg);
    auto y = Number::random(100, prg);
    auto z = x & y;

    REQUIRE(z != x);
    REQUIRE(z != y);
    REQUIRE(z == (y & x));

    x &= y;
    REQUIRE(z != y);
    REQUIRE(z == x);
  }
}

TEST_CASE("Number complement", "[math]") {
  Number a(55452);
  REQUIRE(~a == Number(~55452));
  REQUIRE(~~a == a);
}

TEST_CASE("Number test bit", "[math]") {
  Number a(49);
  // out-of-range returns false
  REQUIRE_FALSE(a.testBit(100));

  REQUIRE(a.testBit(0));
  REQUIRE_FALSE(a.testBit(1));
  REQUIRE_FALSE(a.testBit(2));
  REQUIRE_FALSE(a.testBit(3));
  REQUIRE(a.testBit(4));
  REQUIRE(a.testBit(5));
}

TEST_CASE("Number mod inverse invalid", "[math]") {
  Number a(10);
  REQUIRE_THROWS_MATCHES(math::modInverse(a, Number(0)),
                         std::invalid_argument,
                         Catch::Matchers::Message("modulus cannot be 0"));

  REQUIRE_THROWS_MATCHES(math::modInverse(a, Number(2)),
                         std::logic_error,
                         Catch::Matchers::Message("number not invertible"));
}

TEST_CASE("Number read/write", "[math]") {
  Number a(1234);

  REQUIRE(a.bitSize() == 11);
  REQUIRE(a.byteSize() == 2);

  auto buf =
      std::make_unique<unsigned char[]>(a.byteSize() + sizeof(std::uint32_t));

  a.write(buf.get());
  REQUIRE(a == Number::read(buf.get()));

  auto prg = util::PRG::create("rw");
  REPEAT {
    const auto x = Number::random(100, prg);
    auto bufx =
        std::make_unique<unsigned char[]>(x.byteSize() + sizeof(std::uint32_t));
    x.write(bufx.get());
    REQUIRE(x == Number::read(bufx.get()));
  }
}

TEST_CASE("Number RSA example", "[math]") {
  auto prg = util::PRG::create("rsa");
  const auto p = Number::randomPrime(512, prg);
  const auto q = Number::randomPrime(512, prg);
  REQUIRE(p != q);
  const auto n = p * q;
  const auto lm = math::lcm(p - Number(1), q - Number(1));

  const auto e = Number(0x10001);
  REQUIRE(math::gcd(e, lm) == Number(1));

  const auto d = math::modInverse(e, lm);
  REQUIRE((d * e) % lm == Number(1));

  Number msg(1234);

  const auto ctxt = math::modExp(msg, e, n);
  REQUIRE(ctxt != msg);

  const auto ptxt = math::modExp(ctxt, d, n);
  REQUIRE(ptxt == msg);
}
