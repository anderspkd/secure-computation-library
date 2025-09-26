/* SCL --- Secure Computation Library
 * Copyright (C) 2025 Anders Dalskov
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
#include <stdexcept>

#include "../gf7.h"
#include "scl/math/ff.h"
#include "scl/math/lagrange.h"
#include "scl/math/mersenne61.h"
#include "scl/math/poly.h"
#include "scl/math/vector.h"
#include "scl/primitives/prg.h"
#include "scl/ss/shamir.h"

using namespace scl;

using Elem = FF<Mersenne61>;

TEST_CASE("Shamir share passive", "[ss]") {
  auto prg = PRG::create("shamir passive");
  const auto shares = shamirSecretShare(Elem(123), 3, 4, prg);

  REQUIRE(shares.size() == 4);
  REQUIRE(shamirRecoverP(shares) == Elem(123));
}

TEST_CASE("Shamir reconstruct", "[ss]") {
  auto prg = PRG::create("shamir recons");
  const auto shares = shamirSecretShare(Elem(123), 5, 100, prg);

  REQUIRE(shares.size() == 100);

  const auto lb_0 = computeLagrangeBasis<Elem>(
      {Elem(4), Elem(5), Elem(6), Elem(7), Elem(8), Elem(9)},
      0);
  const auto r_0 =
      innerProd<Elem>(shares.begin() + 3, shares.begin() + 9, lb_0.begin());
  const auto r_0_alt = shares.subVector(3, 9).dot(lb_0);

  REQUIRE(r_0 == Elem(123));
  REQUIRE(r_0_alt == r_0);

  const auto lb_27 = computeLagrangeBasis<Elem>(
      {Elem(4), Elem(5), Elem(6), Elem(7), Elem(8), Elem(9)},
      27);

  const auto r_27 =
      innerProd<Elem>(shares.begin() + 3, shares.begin() + 9, lb_27.begin());
  REQUIRE(r_27 == shares[26]);
}

TEST_CASE("Shamir reconstruct detect", "[ss]") {
  auto prg = PRG::create("shamir detect");
  auto shares = shamirSecretShare(Elem(123), 4, 9, prg);

  REQUIRE(shamirRecoverD(shares, 4) == Elem(123));

  shares[2] = Elem(4);
  REQUIRE_THROWS_MATCHES(
      shamirRecoverD(shares, 4),
      std::logic_error,
      Catch::Matchers::Message("error detected during recovery"));
}

namespace {

Vector<Elem> shareWithDifferentAlphas(PRG& prg, std::size_t t, std::size_t n) {
  auto c = Vector<Elem>::random(t + 1, prg);
  c[0] = Elem(123);
  const auto p = Polynomial<Elem>::create(c);

  std::vector<Elem> shares;
  shares.reserve(n);
  for (std::size_t i = 0; i < n; ++i) {
    shares.emplace_back(p.evaluate(Elem{(int)i + 42}));
  }
  return Vector<Elem>(shares);
}

}  // namespace

TEST_CASE("Shamir reconstruct different x and alphas", "[ss]") {
  auto prg = PRG::create("shamir detect2");

  const auto shares = shareWithDifferentAlphas(prg, 3, 7);
  const auto alphas = Vector<Elem>::range(42, 50);

  REQUIRE(shamirRecoverD(shares, alphas, 3, 3, Elem(0)) == Elem(123));

  REQUIRE(shamirRecoverD(shares, alphas, 3, 3, alphas[0]) == shares[0]);
}

TEST_CASE("Shamir reconstruct correct", "[sim]") {
  auto prg = PRG::create("shamir correct");
  auto shares = shamirSecretShare(Elem(123), 2, 7, prg);

  REQUIRE(shamirRecoverC(shares).f.evaluate(Elem{0}) == Elem(123));

  shares[0] = Elem(22);
  shares[1] = Elem(23);

  REQUIRE(shamirRecoverC(shares).f.evaluate(Elem{0}) == Elem(123));

  shares[2] = Elem(24);

  REQUIRE_THROWS_MATCHES(shamirRecoverC(shares),
                         std::logic_error,
                         Catch::Matchers::Message("could not correct shares"));
}

TEST_CASE("Shamir reconstruct correct different alphas", "[ss]") {
  auto prg = PRG::create("shamir correct2");

  auto shares = shareWithDifferentAlphas(prg, 2, 7);
  const auto alphas = Vector<Elem>::range(42, 50);

  REQUIRE(shamirRecoverC(shares, alphas).f.constantTerm() == Elem(123));

  shares[4] = Elem(5555);

  const auto r = shamirRecoverC(shares, alphas);
  REQUIRE(r.f.constantTerm() == Elem(123));
  REQUIRE(r.err.evaluate(alphas[4]) == Elem(0));
}

TEST_CASE("BerlekampWelch wiki reference test", "[ss][math]") {
  // https://en.wikipedia.org/wiki/Berlekamp%E2%80%93Welch_algorithm#Example

  using Elem = FF<test::GaloisField7>;

  Vector bs = {Elem(1), Elem(5), Elem(3), Elem(6), Elem(3), Elem(2), Elem(2)};
  Vector corrected =
      {Elem(1), Elem(6), Elem(3), Elem(6), Elem(1), Elem(2), Elem(2)};

  auto s = shamirRecoverC(bs);
  // errors
  REQUIRE(s.err.evaluate(Elem(2)) == Elem::zero());
  REQUIRE(s.err.evaluate(Elem(5)) == Elem::zero());

  for (std::size_t i = 0; i < bs.size(); ++i) {
    REQUIRE(s.f.evaluate(Elem(i + 1)) == corrected[i]);
  }
}
