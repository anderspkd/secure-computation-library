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
#include <stdexcept>

#include "scl/math/curves/secp256k1.h"
#include "scl/math/ec_ops.h"
#include "scl/math/fp.h"
#include "scl/math/number.h"
#include "scl/util/prg.h"

using namespace scl;

using Curve = math::EC<math::Secp256k1>;
using Scalar = Curve::Order;
using Field = Curve::Field;

namespace {

Curve RandomPoint(util::PRG& prg) {
  auto r = math::Number::Random(100, prg);
  return Curve::Generator() * r;
}
}  // namespace

TEST_CASE("Secp256k1 defs", "[math][ff]") {
  REQUIRE(std::string(Field::Name()) == "secp256k1_field");
  REQUIRE(Field::ByteSize() == 32);
  REQUIRE(Field::BitSize() == 256);

  REQUIRE(std::string(Scalar::Name()) == "secp256k1_order");
  REQUIRE(Scalar::ByteSize() == 32);
  REQUIRE(Scalar::BitSize() == 256);

  REQUIRE(std::string(Curve::Name()) == "secp256k1");
  REQUIRE(Curve::ByteSize() == 33);
  REQUIRE(Curve::ByteSize(false) == 65);
  REQUIRE(Curve::BitSize() == 264);
  REQUIRE(Curve::BitSize(false) == 520);
}

TEST_CASE("Secp256k1 field to string", "[math][ff]") {
  REQUIRE(Field(0).ToString() == "0");

  auto prg = util::PRG::Create("Secp256k1 field");
  auto x = Field::Random(prg);
  REQUIRE(x.ToString() ==
          "62883be8479ee8f4a3367086d0044440bc7505bc2a2b099e3f71f131eedd42d7");
}

TEST_CASE("Secp256k1 field from string", "[math][ff]") {
  Field y = Field::FromString("cafe");
  REQUIRE(y == Field(51966));
  REQUIRE(y.ToString() == "cafe");

  Field x = Field::FromString(
      "fffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2a");
  REQUIRE(x.ToString() ==
          "fffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2a");

  std::stringstream ss;
  ss << x;
  REQUIRE(ss.str() == x.ToString());

  Field twoPast = Field::FromString(
      "fffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc31");
  REQUIRE(twoPast == Field(2));

  REQUIRE(Field::FromString("016") == Field::FromString("16"));

  REQUIRE_THROWS_MATCHES(
      Field::FromString("ffffffffffffffffffffffffffffffffffffffffffffffffffff"
                        "fffffffffffff"),
      std::invalid_argument,
      Catch::Matchers::Message("hex string too large to parse"));
}

TEST_CASE("Secp256k1 from affine", "[math][ec]") {
  auto x = Field::FromString(
      "e47b4a1c2e13cf0e97c9adf5a645ce388e04317b7830401aabb42e188c9883fa");
  auto y = Field::FromString(
      "2aafa6e870684327ec92006e6c601a8b6e0fb9ff06ae120cb330a2eee86009ff");
  auto g = Curve::FromAffine(x, y);
  REQUIRE(!g.PointAtInfinity());

  auto as_affine = g.ToAffine();
  REQUIRE(as_affine[0] == x);
  REQUIRE(as_affine[1] == y);

  REQUIRE_THROWS_MATCHES(
      Curve::FromAffine(Field(0), Field(0)),
      std::invalid_argument,
      Catch::Matchers::Message("provided (x, y) not on curve"));
}

TEST_CASE("Secp256k1 point-at-infinity", "[math][ec]") {
  Curve p;
  REQUIRE(p.PointAtInfinity());
}

TEST_CASE("Secp256k1 generator", "[math][ec]") {
  auto g = Curve::Generator();

  REQUIRE(
      g.ToString() ==
      "EC{79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798, "
      "483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8}");

  std::stringstream ss;
  ss << g;
  REQUIRE(ss.str() == g.ToString());

  auto ord = math::Number::FromString(
      "fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141");

  REQUIRE(!g.PointAtInfinity());
  auto poi = g * ord;
  REQUIRE(poi.PointAtInfinity());
  auto not_poi = g * (ord - math::Number(1));
  REQUIRE(poi != not_poi);
  REQUIRE(!not_poi.PointAtInfinity());

  REQUIRE(poi.ToString() == "EC{POINT_AT_INFINITY}");
}

TEST_CASE("Secp256k1 addition", "[math][ec]") {
  auto prg = util::PRG::Create("Secp256k1 addition");

  auto a = RandomPoint(prg);
  auto b = RandomPoint(prg);
  REQUIRE(a != b);
  auto c = a + b;
  REQUIRE(a != c);
  REQUIRE(b != c);
  auto d = a + a;
  REQUIRE(d == a.Double());
  a += b;
  REQUIRE(a == c);
  REQUIRE(a != b);
  REQUIRE((c - a).PointAtInfinity());
}

TEST_CASE("Secp256k1 negation", "[math][ec]") {
  auto prg = util::PRG::Create("Secp256k1 negation");

  auto a = RandomPoint(prg);
  auto b = -a;
  REQUIRE((a + b).PointAtInfinity());
}

TEST_CASE("Secp256k1 scalar multiplication", "[math][ec]") {
  auto prg = util::PRG::Create("Secp256k1 scalar-mul");

  auto a = RandomPoint(prg);
  auto p_minus_1 = Scalar(-1);

  auto c = a * p_minus_1;
  REQUIRE(!c.PointAtInfinity());
  REQUIRE((c + a).PointAtInfinity());

  auto x = Scalar::Random(prg);
  auto y = Scalar::Random(prg);
  REQUIRE((x + y) * a == x * a + y * a);

  auto G = Curve::Generator();

  auto v = Scalar::FromString("03");
  auto u = Scalar::FromString("02");
  auto w = Scalar::FromString("06");

  auto P = G * w;
  auto Q = (G * v) * u;

  REQUIRE(P == Q);

  auto n = math::Number::FromString("06");
  REQUIRE(n * G == w * G);
  REQUIRE(n * G == G * n);
}

TEST_CASE("Secp256k1 negation special case", "[math][ec]") {
  using CurveT = math::Secp256k1;
  CurveT::ValueType point = {Field(1), Field(0), Field(1)};
  REQUIRE(!math::CurveIsPointAtInfinity<CurveT>(point));
  math::CurveNegate<CurveT>(point);
  REQUIRE(math::CurveIsPointAtInfinity<CurveT>(point));
}

TEST_CASE("Secp256k1 double point special case", "[math][ec]") {
  using CurveT = math::Secp256k1;
  CurveT::ValueType point = {Field(1), Field(0), Field(1)};
  math::CurveDouble<CurveT>(point);
  REQUIRE(math::CurveIsPointAtInfinity<CurveT>(point));
}

TEST_CASE("Secp256k1 serialization", "[math][ec]") {
  auto prg = util::PRG::Create();

  REQUIRE(Curve::ByteSize(false) == 32 + 32 + 1);
  REQUIRE(Curve::ByteSize(true) == 32 + 1);

  auto a = RandomPoint(prg);
  auto buffer = std::make_unique<unsigned char[]>(Curve::ByteSize(false));
  a.Write(buffer.get(), false);
  REQUIRE(buffer[0] == 0x04);
  auto c = Curve::Read(buffer.get());
  REQUIRE(a == c);

  a.Write(buffer.get(), true);
  auto d = Curve::Read(buffer.get());
  REQUIRE(buffer[0] == 0x01);
  REQUIRE(a == d);

  Curve poi;
  poi.Write(buffer.get(), false);
  REQUIRE(buffer[0] == 0x06);
  auto e = Curve::Read(buffer.get());
  REQUIRE(e.PointAtInfinity());

  poi.Write(buffer.get(), true);
  REQUIRE(buffer[0] == 0x02);
  auto f = Curve::Read(buffer.get());
  REQUIRE(f.PointAtInfinity());

  auto g = Curve::FromAffine(
      Field::FromString("e47b4a1c2e13cf0e97c9adf5a645ce388e04317b7830401aabb4"
                        "2e188c9883fa"),  //
      Field::FromString("2aafa6e870684327ec92006e6c601a8b6e0fb9ff06ae120cb330"
                        "a2eee86009ff")  //
  );
  g.Write(buffer.get());
  REQUIRE(buffer[0] == 0x01);
  auto h = Curve::Read(buffer.get());
  REQUIRE(h == g);

  auto i = Curve::FromAffine(
      Field::FromString("b2d352841ef12627042948c3b3d4ed822fc99a4643d446f8ab9b"
                        "de5aa5f63d36"),  //
      Field::FromString("f1a6bd63f76bb38cf80a1d88da5167c1b102288dbab9b04c210b"
                        "d9863d83d0e3")  //
  );
  i.Write(buffer.get());
  auto j = Curve::Read(buffer.get());
  REQUIRE(i == j);
}
