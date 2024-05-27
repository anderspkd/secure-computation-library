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
#include <stdexcept>

#include "scl/math/curves/secp256k1.h"
#include "scl/math/ec.h"
#include "scl/math/number.h"
#include "scl/util/digest.h"
#include "scl/util/hash.h"
#include "scl/util/prg.h"

using namespace scl;

using Curve = math::EC<math::ec::Secp256k1>;
using Scalar = Curve::ScalarField;
using Field = Curve::Field;

namespace {

Curve randomPoint(util::PRG& prg) {
  auto r = math::Number::random(100, prg);
  return Curve::generator() * r;
}
}  // namespace

TEST_CASE("Secp256k1 defs", "[math][ff]") {
  REQUIRE(std::string(Field::name()) == "secp256k1_field");
  REQUIRE(Field::byteSize() == 32);
  REQUIRE(Field::bitSize() == 256);

  REQUIRE(std::string(Scalar::name()) == "secp256k1_order");
  REQUIRE(Scalar::byteSize() == 32);
  REQUIRE(Scalar::bitSize() == 256);

  REQUIRE(std::string(Curve::name()) == "secp256k1");
  REQUIRE(Curve::byteSize(true) == 33);
  REQUIRE(Curve::byteSize(false) == 65);
  REQUIRE(Curve::bitSize(true) == 264);
  REQUIRE(Curve::bitSize(false) == 520);
}

TEST_CASE("Secp256k1 field to string", "[math][ff]") {
  REQUIRE(Field(0).toString() == "0");

  auto prg = util::PRG::create("Secp256k1 field");
  auto x = Field::random(prg);
  REQUIRE(x.toString() ==
          "62883be8479ee8f4a3367086d0044440bc7505bc2a2b099e3f71f131eedd42d7");
}

TEST_CASE("Secp256k1 field from string", "[math][ff]") {
  Field y = Field::fromString("cafe");
  REQUIRE(y == Field(51966));
  REQUIRE(y.toString() == "cafe");

  Field x = Field::fromString(
      "fffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2a");
  REQUIRE(x.toString() ==
          "fffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2a");

  std::stringstream ss;
  ss << x;
  REQUIRE(ss.str() == x.toString());

  Field twoPast = Field::fromString(
      "fffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc31");
  REQUIRE(twoPast == Field(2));

  REQUIRE(Field::fromString("016") == Field::fromString("16"));

  REQUIRE_THROWS_MATCHES(
      Field::fromString("ffffffffffffffffffffffffffffffffffffffffffffffffffff"
                        "fffffffffffff"),
      std::invalid_argument,
      Catch::Matchers::Message("hex string too large to parse"));
}

TEST_CASE("Secp256k1 from affine", "[math][ec]") {
  auto x = Field::fromString(
      "e47b4a1c2e13cf0e97c9adf5a645ce388e04317b7830401aabb42e188c9883fa");
  auto y = Field::fromString(
      "2aafa6e870684327ec92006e6c601a8b6e0fb9ff06ae120cb330a2eee86009ff");
  auto g = Curve::fromAffine(x, y);
  REQUIRE(!g.isPointAtInfinity());

  auto as_affine = g.toAffine();
  REQUIRE(as_affine[0] == x);
  REQUIRE(as_affine[1] == y);

  REQUIRE_THROWS_MATCHES(
      Curve::fromAffine(Field(0), Field(0)),
      std::invalid_argument,
      Catch::Matchers::Message("provided (x, y) not on curve"));
}

TEST_CASE("Secp256k1 point-at-infinity", "[math][ec]") {
  Curve p;
  REQUIRE(p.isPointAtInfinity());
}

TEST_CASE("Secp256k1 generator", "[math][ec]") {
  auto g = Curve::generator();

  REQUIRE(
      g.toString() ==
      "EC{79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798, "
      "483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8}");

  std::stringstream ss;
  ss << g;
  REQUIRE(ss.str() == g.toString());

  auto ord = math::order<Scalar>();

  REQUIRE(!g.isPointAtInfinity());
  auto poi = g * ord;
  REQUIRE(poi.isPointAtInfinity());
  auto not_poi = g * (ord - math::Number(1));
  REQUIRE(poi != not_poi);
  REQUIRE(!not_poi.isPointAtInfinity());

  REQUIRE(poi.toString() == "EC{POINT_AT_INFINITY}");
}

TEST_CASE("Secp256k1 addition", "[math][ec]") {
  auto prg = util::PRG::create("Secp256k1 addition");

  auto a = randomPoint(prg);
  auto b = randomPoint(prg);
  REQUIRE(a != b);
  auto c = a + b;
  REQUIRE(a != c);
  REQUIRE(b != c);
  auto d = a + a;
  REQUIRE(d == a.doublePoint());
  a += b;
  REQUIRE(a == c);
  REQUIRE(a != b);
  REQUIRE((c - a).isPointAtInfinity());

  auto x = randomPoint(prg);
  auto y = randomPoint(prg);
  auto z = x + y;
  x.normalize();
  REQUIRE(x + y == y + x);
  REQUIRE(z == x + y);
}

TEST_CASE("Secp256k1 negation", "[math][ec]") {
  auto prg = util::PRG::create("Secp256k1 negation");

  auto a = randomPoint(prg);
  auto b = -a;
  REQUIRE((a + b).isPointAtInfinity());
}

TEST_CASE("Secp256k1 scalar multiplication", "[math][ec]") {
  auto prg = util::PRG::create("Secp256k1 scalar-mul");

  auto a = randomPoint(prg);
  auto p_minus_1 = Scalar(-1);

  auto c = a * p_minus_1;
  REQUIRE(!c.isPointAtInfinity());
  REQUIRE((c + a).isPointAtInfinity());

  auto x = Scalar::random(prg);
  auto y = Scalar::random(prg);
  REQUIRE((x + y) * a == x * a + y * a);

  auto G = Curve::generator();

  auto v = Scalar::fromString("03");
  auto u = Scalar::fromString("02");
  auto w = Scalar::fromString("06");

  auto P = G * w;
  auto Q = (G * v) * u;

  REQUIRE(P == Q);

  auto n = math::Number::fromString("06");
  REQUIRE(n * G == w * G);
  REQUIRE(n * G == G * n);
}

TEST_CASE("Secp256k1 negation special case", "[math][ec]") {
  Curve P;
  P.negate();
  REQUIRE(P.isPointAtInfinity());
}

TEST_CASE("Secp256k1 serialization", "[math][ec]") {
  auto prg = util::PRG::create();

  REQUIRE(Curve::byteSize(false) == 32 + 32 + 1);
  REQUIRE(Curve::byteSize(true) == 32 + 1);

  auto a = randomPoint(prg);
  auto buffer = std::make_unique<unsigned char[]>(Curve::byteSize(false));
  a.write(buffer.get(), false);
  REQUIRE(buffer[0] == 0x04);
  auto c = Curve::read(buffer.get());
  REQUIRE(a == c);

  a.write(buffer.get(), true);
  auto d = Curve::read(buffer.get());
  REQUIRE(buffer[0] == 0x01);
  REQUIRE(a == d);

  Curve poi;
  poi.write(buffer.get(), false);
  REQUIRE(buffer[0] == 0x06);
  auto e = Curve::read(buffer.get());
  REQUIRE(e.isPointAtInfinity());

  poi.write(buffer.get(), true);
  REQUIRE(buffer[0] == 0x02);
  auto f = Curve::read(buffer.get());
  REQUIRE(f.isPointAtInfinity());

  auto g = Curve::fromAffine(
      Field::fromString("e47b4a1c2e13cf0e97c9adf5a645ce388e04317b7830401aabb4"
                        "2e188c9883fa"),  //
      Field::fromString("2aafa6e870684327ec92006e6c601a8b6e0fb9ff06ae120cb330"
                        "a2eee86009ff")  //
  );
  g.write(buffer.get(), true);
  REQUIRE(buffer[0] == 0x01);
  auto h = Curve::read(buffer.get());
  REQUIRE(h == g);

  auto i = Curve::fromAffine(
      Field::fromString("b2d352841ef12627042948c3b3d4ed822fc99a4643d446f8ab9b"
                        "de5aa5f63d36"),  //
      Field::fromString("f1a6bd63f76bb38cf80a1d88da5167c1b102288dbab9b04c210b"
                        "d9863d83d0e3")  //
  );
  i.write(buffer.get(), true);
  auto j = Curve::read(buffer.get());
  REQUIRE(i == j);
}

TEST_CASE("Secp256k1 order", "[math]") {
  auto ord = math::order<Field>();
  REQUIRE(
      ord ==
      math::Number::fromString(
          "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F"));
}

TEST_CASE("Secp256k1 hashing", "[math]") {
  util::Hash<256> hgen;

  const auto digest_gen = hgen.update(Curve::generator()).finalize();
  REQUIRE(util::digestToString(digest_gen) ==
          "3f0db2047deb5c2c92e336aecdd4ba1d745fcfd0e77a5f8592dda348a3ff5707");

  util::Hash<256> hpoi;
  const auto digest_poi = hpoi.update(Curve{}).finalize();
  REQUIRE(util::digestToString(digest_poi) ==
          "4fdfe4c2be45edb360dab48435c14be84e087c162cbb421d8c91a3c99e31a82f");
}
