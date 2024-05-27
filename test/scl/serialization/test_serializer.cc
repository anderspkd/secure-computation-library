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

#include "scl/math/fp.h"
#include "scl/math/number.h"
#include "scl/serialization/serializer.h"

using namespace scl;

TEST_CASE("Serialization simple types", "[misc]") {
  using Sint = seri::Serializer<int>;
  const auto int_size = sizeof(int);
  unsigned char buf[4 * int_size];

  REQUIRE(int_size == Sint::sizeOf(10));

  Sint::write(1, buf);
  Sint::write(3, buf + int_size);
  Sint::write(5, buf + 2 * int_size);
  Sint::write(7, buf + 3 * int_size);

  int v;
  Sint::read(v, buf);
  REQUIRE(v == 1);
  Sint::read(v, buf + int_size);
  REQUIRE(v == 3);
  Sint::read(v, buf + 2 * int_size);
  REQUIRE(v == 5);
  Sint::read(v, buf + 3 * int_size);
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

  REQUIRE(Sss::sizeOf(s) == sizeof(SomeStruct));

  Sss::write(s, buf);

  SomeStruct sr;
  Sss::read(sr, buf);

  REQUIRE(s.vi == sr.vi);
  REQUIRE(s.vb == sr.vb);
  REQUIRE(s.vd == sr.vd);
}

constexpr std::size_t VEC_OVERHEAD = sizeof(seri::StlVecSizeType);

TEST_CASE("Serialization vector", "[misc]") {
  using Sv = seri::Serializer<std::vector<int>>;
  std::vector<int> v = {1, 2, 3, 4};

  REQUIRE(Sv::sizeOf(v) == 4 * sizeof(int) + VEC_OVERHEAD);
  unsigned char buf[4 * sizeof(int) + VEC_OVERHEAD];

  Sv::write(v, buf);

  std::vector<int> w;
  Sv::read(w, buf);

  REQUIRE(w == v);
}

TEST_CASE("Serialization vector vector", "[misc]") {
  using Sv = seri::Serializer<std::vector<std::vector<int>>>;
  std::vector<std::vector<int>> v = {{1, 2, 3}, {2, 3}, {5, 6, 7}};

  const auto expected_size = 8 * sizeof(int) + 4 * VEC_OVERHEAD;
  REQUIRE(Sv::sizeOf(v) == expected_size);
  unsigned char buf[expected_size];

  Sv::write(v, buf);

  std::vector<std::vector<int>> w;
  Sv::read(w, buf);

  REQUIRE(v == w);
}

TEST_CASE("Serialization Vec", "[misc]") {
  using Fp = math::Fp<61>;
  using Sv = seri::Serializer<std::vector<Fp>>;

  std::vector<Fp> v = {Fp(1), Fp(2), Fp(3)};
  const auto expected_size = VEC_OVERHEAD + Fp::byteSize() * 3;
  REQUIRE(Sv::sizeOf(v) == expected_size);

  unsigned char buf[expected_size];

  Sv::write(v, buf);

  std::vector<Fp> w;
  Sv::read(w, buf);

  REQUIRE(v == w);
}

TEST_CASE("Serialization number", "[misc]") {
  using Sn = seri::Serializer<math::Number>;

  math::Number a(1234);
  auto buf = std::make_unique<unsigned char[]>(Sn::sizeOf(a));

  Sn::write(a, buf.get());
  math::Number b;
  Sn::read(b, buf.get());

  REQUIRE(a == b);
}

TEST_CASE("Serialization number vector", "[misc]") {
  using Sn = seri::Serializer<std::vector<math::Number>>;

  std::vector<math::Number> nums = {math::Number(22222123),
                                    math::Number(123),
                                    math::Number(-10)};

  auto buf = std::make_unique<unsigned char[]>(Sn::sizeOf(nums));
  Sn::write(nums, buf.get());

  std::vector<math::Number> r;
  Sn::read(r, buf.get());

  REQUIRE(nums == r);
}
