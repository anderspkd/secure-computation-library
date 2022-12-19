/**
 * @file test_mersenne127.cc
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

#include "scl/math/fp.h"

using Field = scl::Fp<127>;
using u128 = __uint128_t;

TEST_CASE("Mersenne127", "[math]") {
  Field zero = Field::Zero();
  Field one = Field::One();
  Field x(0x7b);
  Field big = Field::FromString("58797a14d0653d22a05c11c60e1aacf4");

  SECTION("Name") {
    REQUIRE(std::string(Field::Name()) == "Mersenne127");
  }

  SECTION("ToString") {
    REQUIRE(zero.ToString() == "0");
    REQUIRE(one.ToString() == "1");
    REQUIRE(x.ToString() == "7b");
    REQUIRE(big.ToString() == "58797a14d0653d22a05c11c60e1aacf4");
    std::stringstream ss;
    ss << x;
    REQUIRE(ss.str() == "7b");
  }

  SECTION("Construction") {
    REQUIRE(Field::FromString("80000000000000000000000000000000") ==
            Field::One());
  }

  SECTION("Sizes") {
    REQUIRE(Field::BitSize() == 127);
    REQUIRE(Field::ByteSize() == 16);
  }

  SECTION("Read/Write") {
    unsigned char buffer[Field::ByteSize()];
    big.Write(buffer);
    auto y = Field::Read(buffer);
    REQUIRE(big == y);

    x.Write(buffer);
    auto z = Field::Read(buffer);
    REQUIRE(z == x);
  }

  SECTION("FromString") {
    auto y = Field::FromString("58797a14d0653d22a05c11c60e1aacf4");
    REQUIRE(y == big);
  }
}
