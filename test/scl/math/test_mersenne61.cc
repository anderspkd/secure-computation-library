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

#include "scl/math/fp.h"

using namespace scl;

using Field = math::Fp<61>;

TEST_CASE("Mersenne61 defs", "[math][ff]") {
  REQUIRE(std::string(Field::name()) == "Mersenne61");
  REQUIRE(Field::bitSize() == 61);
  REQUIRE(Field::byteSize() == 8);
}

TEST_CASE("Mersenne61 to string", "[math][ff]") {
  Field zero = Field::zero();
  Field one = Field::one();
  Field x(0x7b);
  Field big(0x41621e);

  REQUIRE(zero.toString() == "0");
  REQUIRE(one.toString() == "1");
  REQUIRE(x.toString() == "7b");
  REQUIRE(big.toString() == "41621e");
  std::stringstream ss;
  ss << x;
  REQUIRE(ss.str() == "7b");
}

TEST_CASE("Mersenne61 from string", "[math][ff]") {
  Field x(0x7b);
  Field big(0x41621e);

  REQUIRE_THROWS_MATCHES(Field::fromString("012"),
                         std::invalid_argument,
                         Catch::Matchers::Message("odd-length hex string"));
  REQUIRE_THROWS_MATCHES(
      Field::fromString("1g"),
      std::invalid_argument,
      Catch::Matchers::Message("encountered invalid hex character"));
  auto y = Field::fromString("7b");
  REQUIRE(x == y);
  auto z = Field::fromString("41621E");
  REQUIRE(z == big);
}

TEST_CASE("Mersenne61 read/write", "[math][ff]") {
  Field x(0x7b);
  Field big(0x41621e);

  unsigned char buffer[Field::byteSize()];
  x.write(buffer);
  auto y = Field::read(buffer);
  REQUIRE(x == y);
  big.write(buffer);
  auto z = Field::read(buffer);
  REQUIRE(z == big);
}
