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

#include "scl/math/fp.h"

using namespace scl;

using Field = math::Fp<61>;

TEST_CASE("Mersenne61 defs", "[math][ff]") {
  REQUIRE(std::string(Field::Name()) == "Mersenne61");
  REQUIRE(Field::BitSize() == 61);
  REQUIRE(Field::ByteSize() == 8);
}

TEST_CASE("Mersenne61 to string", "[math][ff]") {
  Field zero = Field::Zero();
  Field one = Field::One();
  Field x(0x7b);
  Field big(0x41621e);

  REQUIRE(zero.ToString() == "0");
  REQUIRE(one.ToString() == "1");
  REQUIRE(x.ToString() == "7b");
  REQUIRE(big.ToString() == "41621e");
  std::stringstream ss;
  ss << x;
  REQUIRE(ss.str() == "7b");
}

TEST_CASE("Mersenne61 from string", "[math][ff]") {
  Field x(0x7b);
  Field big(0x41621e);

  REQUIRE_THROWS_MATCHES(Field::FromString("012"),
                         std::invalid_argument,
                         Catch::Matchers::Message("odd-length hex string"));
  REQUIRE_THROWS_MATCHES(
      Field::FromString("1g"),
      std::invalid_argument,
      Catch::Matchers::Message("encountered invalid hex character"));
  auto y = Field::FromString("7b");
  REQUIRE(x == y);
  auto z = Field::FromString("41621E");
  REQUIRE(z == big);
}

TEST_CASE("Mersenne61 read/write", "[math][ff]") {
  Field x(0x7b);
  Field big(0x41621e);

  unsigned char buffer[Field::ByteSize()];
  x.Write(buffer);
  auto y = Field::Read(buffer);
  REQUIRE(x == y);
  big.Write(buffer);
  auto z = Field::Read(buffer);
  REQUIRE(z == big);
}
