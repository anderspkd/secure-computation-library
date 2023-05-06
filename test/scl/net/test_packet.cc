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
#include <iostream>

#include "scl/math/curves/secp256k1.h"
#include "scl/math/fp.h"
#include "scl/math/mat.h"
#include "scl/math/number.h"
#include "scl/net/packet.h"

using namespace scl;

using SmallObj = math::Fp<61>;
using LargeObj = math::FF<math::Secp256k1::Field>;

TEST_CASE("Packet read/write different types", "[net]") {
  net::Packet p;
  p << LargeObj(1234) << SmallObj(33) << LargeObj(5);

  REQUIRE(p.Read<LargeObj>() == LargeObj(1234));
  REQUIRE(p.Read<SmallObj>() == SmallObj(33));
  REQUIRE(p.Read<LargeObj>() == LargeObj(5));
}

TEST_CASE("Packet read/write many", "[net]") {
  net::Packet p;

  for (std::size_t i = 0; i < 10000; ++i) {
    p << SmallObj((int)i);
  }

  REQUIRE(p.Size() == SmallObj::ByteSize() * 10000);

  bool all_equal = true;
  for (std::size_t i = 0; i < 10000; ++i) {
    all_equal &= p.Read<SmallObj>() == SmallObj((int)i);
  }

  REQUIRE(all_equal);
}

TEST_CASE("Packet read/write matrix", "[net]") {
  net::Packet p;

  auto prg = util::PRG::Create("packet mat");
  const auto m = math::Mat<SmallObj>::Random(10, 3, prg);

  p << m;
  REQUIRE(p.Read<math::Mat<SmallObj>>().Equals(m));
}

TEST_CASE("Packet read/write vec", "[net]") {
  net::Packet p;

  auto prg = util::PRG::Create("packet vec");
  const auto v = math::Vec<LargeObj>::Random(10, prg);

  p << v;
  REQUIRE(p.Read<math::Vec<LargeObj>>() == v);
}

TEST_CASE("Packet read/write pointers", "[net]") {
  net::Packet p;

  p << 1 << 2 << 3 << 4;

  REQUIRE(p.Read<int>() == 1);
  REQUIRE(p.Read<int>() == 2);
  p.ResetReadPtr();
  REQUIRE(p.Read<int>() == 1);
  REQUIRE(p.Read<int>() == 2);

  p.ResetWritePtr();
  p << 5 << 6;
  REQUIRE(p.Read<int>() == 5);
  REQUIRE(p.Read<int>() == 6);
}
