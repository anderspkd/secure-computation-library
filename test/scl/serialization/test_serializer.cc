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

#include "scl/math/fp.h"
#include "scl/serialization/serializers.h"

using namespace scl;

TEST_CASE("Serialization simple types", "[misc]") {
  using Sint = seri::Serializer<int>;
  const auto int_size = sizeof(int);
  unsigned char buf[4 * int_size];

  REQUIRE(int_size == Sint::SizeOf(10));

  Sint::Write(1, buf);
  Sint::Write(3, buf + int_size);
  Sint::Write(5, buf + 2 * int_size);
  Sint::Write(7, buf + 3 * int_size);

  int v;
  Sint::Read(v, buf);
  REQUIRE(v == 1);
  Sint::Read(v, buf + int_size);
  REQUIRE(v == 3);
  Sint::Read(v, buf + 2 * int_size);
  REQUIRE(v == 5);
  Sint::Read(v, buf + 3 * int_size);
  REQUIRE(v == 7);
}

struct SomeStruct {
  int vi;
  bool vb;
  double vd;
};

TEST_CASE("Serialization simple types struct", "[misc]") {
  using Sss = seri::Serializer<SomeStruct>;

  SomeStruct s{1, true, 2.5};
  unsigned char buf[sizeof(SomeStruct)];

  REQUIRE(Sss::SizeOf(s) == sizeof(SomeStruct));

  Sss::Write(s, buf);

  SomeStruct sr;
  Sss::Read(sr, buf);

  REQUIRE(s.vi == sr.vi);
  REQUIRE(s.vb == sr.vb);
  REQUIRE(s.vd == sr.vd);
}

TEST_CASE("Serialization vector", "[misc]") {
  using Sv = seri::Serializer<std::vector<int>>;
  std::vector<int> v = {1, 2, 3, 4};

  REQUIRE(Sv::SizeOf(v) == 4 * sizeof(int) + sizeof(std::size_t));
  unsigned char buf[4 * sizeof(int) + sizeof(std::size_t)];

  Sv::Write(v, buf);

  std::vector<int> w;
  Sv::Read(w, buf);

  REQUIRE(w == v);
}

TEST_CASE("Serialization vector vector", "[misc]") {
  using Sv = seri::Serializer<std::vector<std::vector<int>>>;
  std::vector<std::vector<int>> v = {{1, 2, 3}, {2, 3}, {5, 6, 7}};

  const auto expected_size = 8 * sizeof(int) + 4 * sizeof(std::size_t);
  REQUIRE(Sv::SizeOf(v) == expected_size);
  unsigned char buf[expected_size];

  Sv::Write(v, buf);

  std::vector<std::vector<int>> w;
  Sv::Read(w, buf);

  REQUIRE(v == w);
}

TEST_CASE("Serialization Vec", "[misc]") {
  using Fp = math::Fp<61>;
  using Sv = seri::Serializer<std::vector<Fp>>;

  std::vector<Fp> v = {Fp(1), Fp(2), Fp(3)};
  const auto expected_size = sizeof(std::size_t) + Fp::ByteSize() * 3;
  REQUIRE(Sv::SizeOf(v) == expected_size);

  unsigned char buf[expected_size];

  Sv::Write(v, buf);

  std::vector<Fp> w;
  Sv::Read(w, buf);

  REQUIRE(v == w);
}
