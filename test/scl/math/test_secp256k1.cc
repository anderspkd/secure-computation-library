/**
 * @file test_secp256k1.cc
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
#include <stdexcept>

#include "scl/math/curves/secp256k1.h"
#include "scl/math/ec_ops.h"
#include "scl/math/fp.h"
#include "scl/math/number.h"
#include "scl/primitives/prg.h"

using Curve = scl::EC<scl::details::Secp256k1>;
using Field = Curve::Field;

TEST_CASE("secp256k1_field", "[math]") {
  SECTION("name") {
    REQUIRE(std::string(Field::Name()) == "secp256k1_field");
  }

  SECTION("Strings") {
    REQUIRE(Field(0).ToString() == "0");

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

  SECTION("From affine") {
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

  SECTION("addition") {
    Field a(1231);
    Field b(5555);
    auto c = a + b;
    REQUIRE(c == Field(1231 + 5555));
    REQUIRE(a + b == b + a);

    Field d(-1);
    Field e(1);
    REQUIRE(d + e == Field(0));

    Field x = Field::FromString(
        "fffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2a");
    REQUIRE(x + Field(5) == Field(0));

    REQUIRE(a + a == Field(1231 + 1231));
  }

  SECTION("subtraction") {
    Field a(121);
    Field b(231);
    REQUIRE(b - a == Field(231 - 121));

    auto c = -a;
    REQUIRE((c += a) == Field(0));
    c = c - Field(1);
    REQUIRE(c == Field(-1));
  }

  SECTION("multiply") {
    Field a(333);
    Field b(212);
    REQUIRE(a * b == Field(333 * 212));

    Field x = Field::FromString(
        "fffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2d");
    Field y = Field::FromString(
        "fffffffffffffffffffffffffffffffffffffffffffffffffffffffef94e3c23");
    Field z = Field::FromString("0d638018");
    REQUIRE(x * y == z);

    REQUIRE(x * x == Field(4));
  }

  SECTION("inversion") {
    REQUIRE_THROWS_MATCHES(
        Field(0).Inverse(),
        std::invalid_argument,
        Catch::Matchers::Message("0 not invertible modulo prime"));
    Field one(1);
    REQUIRE(one * one.Inverse() == one);

    Field a = Field::FromString(
        "fffffffffffffffffffffffffffffffffffffffffffffffffffffffef94e3c23");
    REQUIRE(a * a.Inverse() == one);
  }

  SECTION("Serialization") {
    Field a = Field::FromString(
        "fffffffffffffffffffffffffffffffffffffffffffffffffffffffef94e3c23");
    unsigned char buf[Field::ByteSize()];
    a.Write(buf);
    Field b = Field::Read(buf);
    REQUIRE(a == b);
  }
}

using Scalar = scl::FF<scl::details::Secp256k1::Order>;

namespace {

Curve RandomPoint(scl::PRG& prg) {
  auto r = scl::Number::Random(100, prg);
  return Curve::Generator() * r;
}
}  // namespace

TEST_CASE("secp256k1", "[math]") {
  SECTION("name") {
    REQUIRE(std::string(Curve::Name()) == "secp256k1");
  }

  auto ord = scl::Number::FromString(
      "fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141");

  SECTION("Point-at-infinity") {
    Curve p;
    REQUIRE(p.PointAtInfinity());
  }

  SECTION("Generator") {
    auto g = Curve::Generator();

    REQUIRE(
        g.ToString() ==
        "EC{79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798, "
        "483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8}");

    std::stringstream ss;
    ss << g;
    REQUIRE(ss.str() == g.ToString());

    REQUIRE(!g.PointAtInfinity());
    auto poi = g * ord;
    REQUIRE(poi.PointAtInfinity());
    auto not_poi = g * (ord - scl::Number(1));
    REQUIRE(poi != not_poi);
    REQUIRE(!not_poi.PointAtInfinity());

    REQUIRE(poi.ToString() == "EC{POINT_AT_INFINITY}");
  }

  auto prg = scl::PRG::Create();

  SECTION("addition") {
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

  SECTION("negation") {
    auto a = RandomPoint(prg);
    auto b = -a;
    REQUIRE((a + b).PointAtInfinity());
  }

  SECTION("Scalar mul order") {
    auto a = RandomPoint(prg);
    auto b = Scalar::FromString(
        "fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364140");
    REQUIRE(b + Scalar::One() == Scalar::Zero());

    auto c = a * b;
    REQUIRE(!c.PointAtInfinity());

    REQUIRE((c + a).PointAtInfinity());
  }

  SECTION("Scalar mul distributive") {
    auto a = RandomPoint(prg);
    auto b = Scalar::Random(prg);
    auto c = Scalar::Random(prg);
    REQUIRE((b + c) * a == b * a + c * a);
  }

  SECTION("Scalar mul assoc") {
    auto a = Curve::Generator();

    auto v = Scalar::FromString("03");
    auto u = Scalar::FromString("02");
    auto w = Scalar::FromString("06");

    auto P = a * w;
    auto Q = (a * v) * u;

    REQUIRE(P == Q);
  }

  SECTION("negation exceptional") {
    using CurveT = scl::details::Secp256k1;
    CurveT::ValueType point = {Field(1), Field(0), Field(1)};
    REQUIRE(!scl::details::CurveIsPointAtInfinity<CurveT>(point));
    scl::details::CurveNegate<CurveT>(point);
    REQUIRE(scl::details::CurveIsPointAtInfinity<CurveT>(point));
  }

  SECTION("double exceptional") {
    using CurveT = scl::details::Secp256k1;
    CurveT::ValueType point = {Field(1), Field(0), Field(1)};
    scl::details::CurveDouble<CurveT>(point);
    REQUIRE(scl::details::CurveIsPointAtInfinity<CurveT>(point));
  }

  SECTION("serialization") {
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
}
