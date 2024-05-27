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
#include <iostream>

#include "scl/math/fields/secp256k1_field.h"
#include "scl/math/fp.h"
#include "scl/math/matrix.h"
#include "scl/math/number.h"
#include "scl/net/packet.h"
#include "scl/serialization/serializer.h"

using namespace scl;

using SmallObj = math::Fp<61>;
using LargeObj = math::FF<math::ff::Secp256k1Field>;

TEST_CASE("Packet read/write different types", "[net]") {
  net::Packet p;
  p << LargeObj(1234) << SmallObj(33) << LargeObj(5);

  REQUIRE(p.read<LargeObj>() == LargeObj(1234));
  REQUIRE(p.read<SmallObj>() == SmallObj(33));
  REQUIRE(p.read<LargeObj>() == LargeObj(5));
}

TEST_CASE("Packet read/write many", "[net]") {
  net::Packet p;

  for (std::size_t i = 0; i < 10000; ++i) {
    p << SmallObj((int)i);
  }

  REQUIRE(p.size() == SmallObj::byteSize() * 10000);

  bool all_equal = true;
  for (std::size_t i = 0; i < 10000; ++i) {
    all_equal &= p.read<SmallObj>() == SmallObj((int)i);
  }

  REQUIRE(all_equal);
}

TEST_CASE("Packet read/write matrix", "[net]") {
  net::Packet p;

  auto prg = util::PRG::create("packet mat");
  const auto m = math::Matrix<SmallObj>::random(10, 3, prg);

  p << m;
  auto mm = p.read<math::Matrix<SmallObj>>();

  REQUIRE(mm.rows() == m.rows());
  REQUIRE(mm.cols() == m.cols());
  REQUIRE(mm.equals(m));
}

TEST_CASE("Packet read/write vec", "[net]") {
  net::Packet p;

  auto prg = util::PRG::create("packet vec");
  const auto v = math::Vector<LargeObj>::random(10, prg);

  p << v;
  REQUIRE(p.read<math::Vector<LargeObj>>() == v);
}

TEST_CASE("Packet read/write pointers", "[net]") {
  net::Packet p;

  p << 1 << 2 << 3 << 4;

  REQUIRE(p.read<int>() == 1);
  REQUIRE(p.read<int>() == 2);
  p.resetReadPtr();
  REQUIRE(p.read<int>() == 1);
  REQUIRE(p.read<int>() == 2);

  p.resetWritePtr();
  p << 5 << 6;
  REQUIRE(p.read<int>() == 5);
  REQUIRE(p.read<int>() == 6);
}

TEST_CASE("Packet Write", "[net]") {
  net::Packet p;

  const auto w = p.write((int)123);
  REQUIRE(w == seri::Serializer<int>::sizeOf(0));
}

TEST_CASE("Packet concat", "[net]") {
  net::Packet p0;
  net::Packet p1;

  p0 << 1 << 2 << LargeObj(44);
  p1 << 3 << SmallObj(55) << 4;

  const auto p0_sz = p0.size();
  const auto p1_sz = p1.size();

  p0 << p1;

  REQUIRE(p0.read<int>() == 1);
  REQUIRE(p0.read<int>() == 2);
  REQUIRE(p0.read<LargeObj>() == LargeObj(44));
  REQUIRE(p0.read<int>() == 3);
  REQUIRE(p0.read<SmallObj>() == SmallObj(55));
  REQUIRE(p0.read<int>() == 4);
  REQUIRE(p0_sz + p1_sz == p0.size());
}

TEST_CASE("Packet remaining", "[net]") {
  net::Packet p;

  p << 1 << 2 << 3;

  REQUIRE(p.remaining() == p.size());
  p.read<int>();

  REQUIRE(p.remaining() == p.size() - sizeof(int));
  p.read<int>();
  p.read<int>();
  REQUIRE(p.remaining() == 0);
}

TEST_CASE("Packet eq", "[net]") {
  net::Packet p0;
  net::Packet p1;

  REQUIRE(p0 == p1);

  p0 << 2;
  REQUIRE_FALSE(p0 == p1);

  p1 << 2;
  REQUIRE(p0 == p1);

  p1 << 3;
  REQUIRE_FALSE(p0 == p1);

  p1.setWritePtr(sizeof(int));

  REQUIRE(p1 == p0);
}
