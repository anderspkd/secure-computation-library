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

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <stdexcept>

#include "../math/fields.h"
#include "scl/math/curves/secp256k1.h"
#include "scl/math/poly.h"
#include "scl/util/prg.h"

using namespace scl;

TEMPLATE_TEST_CASE("Polynomial construct", "[ss][math]", FIELD_DEFS) {
  using FF = TestType;

  math::Polynomial<FF> p;
  REQUIRE(p.degree() == 0);
  REQUIRE(p[0] == FF());
  REQUIRE(p.isZero());

  math::Polynomial<FF> q(FF(123));
  REQUIRE(q.degree() == 0);
  REQUIRE(q.constantTerm() == FF(123));
  REQUIRE(q[0] == FF(123));

  math::Vector coeff = {FF(1), FF(2), FF(6)};
  auto x = math::Polynomial<FF>::create(coeff);
  REQUIRE(x.degree() == 2);
  REQUIRE(x[0] == FF(1));
  REQUIRE(x[1] == FF(2));
  REQUIRE(x[2] == FF(6));
  REQUIRE(x.coefficients() == coeff);

  math::Vector with_zeros = {FF(1), FF(0), FF(3), FF(0)};
  auto y = math::Polynomial<FF>::create(with_zeros);
  REQUIRE(y.degree() == 2);
  REQUIRE(y[0] == FF(1));
  REQUIRE(y[1] == FF(0));
  REQUIRE(y[2] == FF(3));

  math::Vector<FF> empty;
  auto z = math::Polynomial<FF>::create(empty);
  REQUIRE(z.degree() == 0);
  REQUIRE(z[0] == FF(0));
}

TEMPLATE_TEST_CASE("Polynomial evaluate", "[math][ss]", FIELD_DEFS) {
  using FF = TestType;

  math::Vector coeff = {FF(4), FF(5), FF(1)};
  auto p = math::Polynomial<FF>::create(coeff);
  auto x5 = p.evaluate(FF(5));
  REQUIRE(x5 == FF(54));
}

TEMPLATE_TEST_CASE("Polynomial to string", "[math][ss]", FIELD_DEFS) {
  using FF = TestType;

  math::Vector coeff = {FF(4), FF(5), FF(1)};
  auto p = math::Polynomial<FF>::create(coeff);

  REQUIRE(p.toString() == "f(x) = 4 + 5x + 1x^2");
  REQUIRE(p.toString("g", "y") == "g(y) = 4 + 5y + 1y^2");

  std::stringstream ss;
  ss << p;
  REQUIRE(ss.str() == "f(x) = 4 + 5x + 1x^2");
}

TEMPLATE_TEST_CASE("Polynomial addition", "[math][ss]", FIELD_DEFS) {
  using FF = TestType;

  math::Vector c0 = {FF(1), FF(2), FF(3)};
  math::Vector c1 = {FF(5), FF(3), FF(3), FF(1)};
  auto p = math::Polynomial<FF>::create(c0);
  auto q = math::Polynomial<FF>::create(c1);
  auto e = p.add(q);
  REQUIRE(e.degree() == q.degree());
  REQUIRE(e[0] == FF(6));
  REQUIRE(e[1] == FF(5));
  REQUIRE(e[2] == FF(6));
  REQUIRE(e[3] == FF(1));

  auto d = q.add(p);
  REQUIRE(d[0] == e[0]);
  REQUIRE(d[1] == e[1]);
  REQUIRE(d[2] == e[2]);
  REQUIRE(d[3] == e[3]);

  math::Vector cn = {-FF(1), -FF(2), -FF(3)};
  auto t = math::Polynomial<FF>::create(cn);
  auto w = t.add(p);
  REQUIRE(w.degree() == 0);
}

TEMPLATE_TEST_CASE("Polynomial subtraction", "[math][ss]", FIELD_DEFS) {
  using FF = TestType;

  math::Vector c0 = {FF(1), FF(2), FF(3)};
  math::Vector c1 = {FF(5), FF(3), FF(3), FF(1)};
  auto p = math::Polynomial<FF>::create(c0);
  auto q = math::Polynomial<FF>::create(c1);
  auto e = p.subtract(q);
  REQUIRE(e.degree() == q.degree());
  REQUIRE(e[0] == -FF(4));
  REQUIRE(e[1] == -FF(1));
  REQUIRE(e[2] == FF(0));
  REQUIRE(e[3] == -FF(1));

  auto d = q.subtract(p);
  REQUIRE(-d[0] == e[0]);
  REQUIRE(-d[1] == e[1]);
  REQUIRE(-d[2] == e[2]);
  REQUIRE(-d[3] == e[3]);
}

TEMPLATE_TEST_CASE("Polynomial multiplication", "[math][ss]", FIELD_DEFS) {
  using FF = TestType;

  // (1 + 2x + 3x^2) * (5 + 3x + 3x^2 + x^3)
  //  = 5 + 13x + 24x^2 + 16x^3 + 11x^4 + 3x^5
  math::Vector c0 = {FF(1), FF(2), FF(3)};
  math::Vector c1 = {FF(5), FF(3), FF(3), FF(1)};
  auto p = math::Polynomial<FF>::create(c0);
  auto q = math::Polynomial<FF>::create(c1);
  auto e = p.multiply(q);
  REQUIRE(e.degree() == 5);
  REQUIRE(e[0] == FF(5));
  REQUIRE(e[1] == FF(13));
  REQUIRE(e[2] == FF(24));
  REQUIRE(e[3] == FF(16));
  REQUIRE(e[4] == FF(11));
  REQUIRE(e[5] == FF(3));
}

TEMPLATE_TEST_CASE("Polynomial division", "[math][ss]", FIELD_DEFS) {
  using FF = TestType;

  math::Vector c0 = {FF(1), FF(2), FF(3)};
  math::Vector c1 = {FF(5), FF(3), FF(3), FF(1)};
  auto p = math::Polynomial<FF>::create(c0);
  auto q = math::Polynomial<FF>::create(c1);
  auto e = q.divide(p);
  auto x = p.multiply(e[0]).add(e[1]);

  REQUIRE(x.degree() == q.degree());
  for (std::size_t i = 0; i < x.degree(); ++i) {
    REQUIRE(x[i] == q[i]);
  }

  math::Polynomial<FF> z;
  REQUIRE_THROWS_MATCHES(p.divide(z),
                         std::invalid_argument,
                         Catch::Matchers::Message("division by 0"));

  auto prg = util::PRG::create();
  auto c0_ = math::Vector<FF>::random(10, prg);
  auto c1_ = math::Vector<FF>::random(9, prg);
  auto a = math::Polynomial<FF>::create(c0_);
  auto b = math::Polynomial<FF>::create(c1_);
  auto qr = a.divide(b);

  auto v = b.multiply(qr[0]).add(qr[1]);
  REQUIRE(v == a);
}
