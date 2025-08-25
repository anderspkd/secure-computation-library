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

#include "scl/serialization/serializer.h"
#include "scl/util/bitmap.h"

using namespace scl;

TEST_CASE("Bitmap construct", "[util]") {
  util::Bitmap bm(10);
  REQUIRE(bm.numberOfBlocks() == 2);
  REQUIRE(bm.count() == 0);

  util::Bitmap bm0;
  REQUIRE(bm0.numberOfBlocks() == 1);
  REQUIRE(bm0.count() == 0);
}

TEST_CASE("Bitmap get/set", "[util]") {
  util::Bitmap bm(10);

  bm.set(0, true);
  bm.set(7, true);
  bm.set(8, true);

  REQUIRE(bm.at(0));
  REQUIRE(bm.at(7));
  REQUIRE(bm.at(8));
  REQUIRE_FALSE(bm.at(9));

  REQUIRE(bm.count() == 3);

  bm.set(7, false);
  REQUIRE_FALSE(bm.at(7));

  REQUIRE(bm.count() == 2);
}

TEST_CASE("Bitmap XOR", "[util]") {
  util::Bitmap bm0(10);
  util::Bitmap bm1(10);

  bm0.set(0, true);
  bm1.set(0, true);
  // result will have 0 at position 0

  bm0.set(4, true);
  bm1.set(4, false);
  // result will have 1 at position 4

  auto bm = bm0 ^ bm1;

  REQUIRE(bm.at(0) == false);
  REQUIRE(bm.at(4) == true);
  REQUIRE(bm.at(5) == false);
}

TEST_CASE("Bitmap AND", "[util]") {
  util::Bitmap bm0(10);
  util::Bitmap bm1(10);

  bm0.set(0, true);
  bm1.set(0, true);
  // result will have 1 at position 0

  bm0.set(4, true);
  bm1.set(4, false);
  // result will have 0 at position 4

  auto bm = bm0 & bm1;

  REQUIRE(bm.at(0) == true);
  REQUIRE(bm.at(4) == false);
  REQUIRE(bm.at(5) == false);
}

TEST_CASE("Bitmap OR", "[util]") {
  util::Bitmap bm0(10);
  util::Bitmap bm1(10);

  bm0.set(0, true);
  bm1.set(0, true);
  // result will have 1 at position 0

  bm0.set(4, true);
  bm1.set(4, false);
  // result will have 1 at position 4

  auto bm = bm0 | bm1;

  REQUIRE(bm.at(0) == true);
  REQUIRE(bm.at(4) == true);
  REQUIRE(bm.at(5) == false);
}

TEST_CASE("Bitmap NEG", "[util]") {
  util::Bitmap bm0(10);

  bm0.set(0, true);
  bm0.set(4, true);

  auto bm = ~bm0;

  REQUIRE(bm.at(0) == false);
  REQUIRE(bm.at(4) == false);
  REQUIRE(bm.at(5) == true);
}

TEST_CASE("Bitmap equal", "[util]") {
  util::Bitmap bm0(10);
  util::Bitmap bm1(10);

  REQUIRE(bm0 == bm1);

  bm0.set(3, true);

  REQUIRE(bm0 != bm1);
}

TEST_CASE("Bitmap print", "[util]") {
  util::Bitmap bm(10);

  bm.set(2, true);
  bm.set(9, true);

  std::stringstream ss;
  ss << bm;
  REQUIRE(ss.str() == "0000010000000010");
}

TEST_CASE("Bitmap serialization", "[util]") {
  util::Bitmap bm(10);

  bm.set(3, true);
  bm.set(2, true);
  bm.set(5, true);

  REQUIRE(bm.numberOfBlocks() == 2);

  constexpr std::size_t overhead = sizeof(seri::StlVecSizeType);
  unsigned char buf[2 + overhead];

  REQUIRE(seri::Serializer<util::Bitmap>::sizeOf(bm) == 2 + overhead);

  REQUIRE(seri::Serializer<util::Bitmap>::write(bm, buf) == 2 + overhead);

  util::Bitmap b;
  REQUIRE(seri::Serializer<util::Bitmap>::read(b, buf) == 2 + overhead);

  REQUIRE(b == bm);
}
