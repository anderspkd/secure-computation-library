/**
 * @file test_poly.cc
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
#include <stdexcept>

#include "../math/fields.h"
#include "scl/math.h"
#include "scl/math/curves/secp256k1.h"
#include "scl/primitives/prg.h"
#include "scl/ss/poly.h"

using namespace scl_tests;

TEMPLATE_TEST_CASE("Polynomials", "[ss][math]", FIELD_DEFS) {
  using FF = TestType;
  using Poly = scl::details::Polynomial<FF>;
  using Vec = scl::Vec<FF>;

  SECTION("DefaultConstruct") {
    Poly p;
    REQUIRE(p.Degree() == 0);
    REQUIRE(p[0] == FF());
    REQUIRE(p.IsZero());
  }

  SECTION("ConstantConstruct") {
    Poly p(FF(123));
    REQUIRE(p.Degree() == 0);
    REQUIRE(p.ConstantTerm() == FF(123));
    REQUIRE(p[0] == FF(123));
  }

  SECTION("CoefficientConstruct") {
    scl::Vec coeff = {FF(1), FF(2), FF(6)};
    Poly p = Poly::Create(coeff);
    REQUIRE(p.Degree() == 2);
    REQUIRE(p[0] == FF(1));
    REQUIRE(p[1] == FF(2));
    REQUIRE(p[2] == FF(6));

    scl::Vec with_zeros = {FF(1), FF(0), FF(3), FF(0)};
    Poly q = Poly::Create(with_zeros);
    REQUIRE(q.Degree() == 2);
    REQUIRE(q[0] == FF(1));
    REQUIRE(q[1] == FF(0));
    REQUIRE(q[2] == FF(3));

    scl::Vec<FF> empty;
    Poly g = Poly::Create(empty);
    REQUIRE(g.Degree() == 0);
    REQUIRE(g[0] == FF(0));
  }

  scl::Vec coeff = {FF(4), FF(5), FF(1)};
  Poly p = Poly::Create(coeff);

  SECTION("Evaluate") {
    auto x5 = p.Evaluate(FF(5));
    REQUIRE(x5 == FF(54));
  }

  SECTION("ToString") {
    REQUIRE(p.ToString() == "f(x) = 4 + 5x + 1x^2");
    REQUIRE(p.ToString("g", "y") == "g(y) = 4 + 5y + 1y^2");
  }

  SECTION("Add") {
    scl::Vec c0 = {FF(1), FF(2), FF(3)};
    scl::Vec c1 = {FF(5), FF(3), FF(3), FF(1)};
    auto p = Poly::Create(c0);
    auto q = Poly::Create(c1);
    auto e = p.Add(q);
    REQUIRE(e.Degree() == q.Degree());
    REQUIRE(e[0] == FF(6));
    REQUIRE(e[1] == FF(5));
    REQUIRE(e[2] == FF(6));
    REQUIRE(e[3] == FF(1));

    auto d = q.Add(p);
    REQUIRE(d[0] == e[0]);
    REQUIRE(d[1] == e[1]);
    REQUIRE(d[2] == e[2]);
    REQUIRE(d[3] == e[3]);

    scl::Vec cn = {-FF(1), -FF(2), -FF(3)};
    auto t = Poly::Create(cn);
    auto w = t.Add(p);
    REQUIRE(w.Degree() == 0);
  }

  SECTION("Subtract") {
    scl::Vec c0 = {FF(1), FF(2), FF(3)};
    scl::Vec c1 = {FF(5), FF(3), FF(3), FF(1)};
    auto p = Poly::Create(c0);
    auto q = Poly::Create(c1);
    auto e = p.Subtract(q);
    REQUIRE(e.Degree() == q.Degree());
    REQUIRE(e[0] == -FF(4));
    REQUIRE(e[1] == -FF(1));
    REQUIRE(e[2] == FF(0));
    REQUIRE(e[3] == -FF(1));

    auto d = q.Subtract(p);
    REQUIRE(-d[0] == e[0]);
    REQUIRE(-d[1] == e[1]);
    REQUIRE(-d[2] == e[2]);
    REQUIRE(-d[3] == e[3]);
  }

  SECTION("Multiply") {
    // (1 + 2x + 3x^2) * (5 + 3x + 3x^2 + x^3)
    //  = 5 + 13x + 24x^2 + 16x^3 + 11x^4 + 3x^5
    scl::Vec c0 = {FF(1), FF(2), FF(3)};
    scl::Vec c1 = {FF(5), FF(3), FF(3), FF(1)};
    auto p = Poly::Create(c0);
    auto q = Poly::Create(c1);
    auto e = p.Multiply(q);
    REQUIRE(e.Degree() == 5);
    REQUIRE(e[0] == FF(5));
    REQUIRE(e[1] == FF(13));
    REQUIRE(e[2] == FF(24));
    REQUIRE(e[3] == FF(16));
    REQUIRE(e[4] == FF(11));
    REQUIRE(e[5] == FF(3));
  }

  SECTION("Divide") {
    scl::Vec c0 = {FF(1), FF(2), FF(3)};
    scl::Vec c1 = {FF(5), FF(3), FF(3), FF(1)};
    auto p = Poly::Create(c0);
    auto q = Poly::Create(c1);
    auto e = q.Divide(p);
    auto x = p.Multiply(e[0]).Add(e[1]);

    REQUIRE(x.Degree() == q.Degree());
    for (std::size_t i = 0; i < x.Degree(); ++i) {
      REQUIRE(x[i] == q[i]);
    }

    Poly z;
    REQUIRE_THROWS_MATCHES(p.Divide(z),
                           std::invalid_argument,
                           Catch::Matchers::Message("division by 0"));
  }

  SECTION("DivideRandom") {
    auto prg = scl::PRG::Create();
    auto c0 = Vec::Random(10, prg);
    auto c1 = Vec::Random(9, prg);
    auto a = Poly::Create(c0);
    auto b = Poly::Create(c1);
    auto qr = a.Divide(b);

    auto x = b.Multiply(qr[0]).Add(qr[1]);
    REQUIRE(x.Degree() == a.Degree());
    for (std::size_t i = 0; i < x.Degree(); ++i) {
      REQUIRE(x[i] == a[i]);
    }
  }
}
