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
#include <sstream>

#include "scl/math/fp.h"

using namespace scl;

using Field = math::Fp<127>;
using u128 = __uint128_t;

TEST_CASE("Mersenne127 defs", "[math][ff]") {
  REQUIRE(Field::bitSize() == 127);
  REQUIRE(Field::byteSize() == 16);
  REQUIRE(std::string(Field::name()) == "Mersenne127");
}

TEST_CASE("Mersenne127 to string", "[math][ff]") {
  REQUIRE(Field::zero().toString() == "0");
  REQUIRE(Field::one().toString() == "1");

  Field x(0x7b);
  REQUIRE(x.toString() == "7b");

  REQUIRE(Field::fromString("80000000000000000000000000000000") ==
          Field::one());

  Field big = Field::fromString("58797a14d0653d22a05c11c60e1aacf4");
  REQUIRE(big.toString() == "58797a14d0653d22a05c11c60e1aacf4");

  std::stringstream ss;
  ss << x;
  REQUIRE(ss.str() == "7b");
}

TEST_CASE("Mersenne127 from string", "[math][ff]") {
  auto y = Field::fromString("7b");
  REQUIRE(y == Field(0x7b));
}

TEST_CASE("Mersenne127 read/write", "[math][ff]") {
  Field big = Field::fromString("58797a14d0653d22a05c11c60e1aacf4");
  unsigned char buffer[Field::byteSize()];
  big.write(buffer);
  auto y = Field::read(buffer);
  REQUIRE(big == y);

  Field x(0x7b);
  x.write(buffer);
  auto z = Field::read(buffer);
  REQUIRE(z == x);
}
