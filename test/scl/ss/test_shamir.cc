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
#include <stdexcept>

#include "../gf7.h"
#include "scl/math/fp.h"
#include "scl/math/lagrange.h"
#include "scl/math/poly.h"
#include "scl/math/vector.h"
#include "scl/ss/shamir.h"
#include "scl/util/prg.h"

using namespace scl;

using FF = math::Fp<61>;

TEST_CASE("Shamir share passive", "[ss]") {
  auto prg = util::PRG::create("shamir passive");
  const auto shares = ss::shamirSecretShare(FF(123), 3, 4, prg);

  REQUIRE(shares.size() == 4);
  REQUIRE(ss::shamirRecoverP(shares) == FF(123));
}

TEST_CASE("Shamir reconstruct", "[ss]") {
  auto prg = util::PRG::create("shamir recons");
  const auto shares = ss::shamirSecretShare(FF(123), 5, 100, prg);

  REQUIRE(shares.size() == 100);

  const auto lb_0 =
      math::computeLagrangeBasis<FF>({FF(4), FF(5), FF(6), FF(7), FF(8), FF(9)},
                                     0);
  const auto r_0 =
      math::innerProd<FF>(shares.begin() + 3, shares.begin() + 9, lb_0.begin());
  const auto r_0_alt = shares.subVector(3, 9).dot(lb_0);

  REQUIRE(r_0 == FF(123));
  REQUIRE(r_0_alt == r_0);

  const auto lb_27 =
      math::computeLagrangeBasis<FF>({FF(4), FF(5), FF(6), FF(7), FF(8), FF(9)},
                                     27);

  const auto r_27 = math::innerProd<FF>(shares.begin() + 3,
                                        shares.begin() + 9,
                                        lb_27.begin());
  REQUIRE(r_27 == shares[26]);
}

TEST_CASE("Shamir reconstruct detect", "[ss]") {
  auto prg = util::PRG::create("shamir detect");
  auto shares = ss::shamirSecretShare(FF(123), 4, 9, prg);

  REQUIRE(ss::shamirRecoverD(shares, 4) == FF(123));

  shares[2] = FF(4);
  REQUIRE_THROWS_MATCHES(
      ss::shamirRecoverD(shares, 4),
      std::logic_error,
      Catch::Matchers::Message("error detected during recovery"));
}

namespace {

math::Vector<FF> shareWithDifferentAlphas(util::PRG& prg,
                                          std::size_t t,
                                          std::size_t n) {
  auto c = math::Vector<FF>::random(t + 1, prg);
  c[0] = FF(123);
  const auto p = math::Polynomial<FF>::create(c);

  std::vector<FF> shares;
  shares.reserve(n);
  for (std::size_t i = 0; i < n; ++i) {
    shares.emplace_back(p.evaluate(FF{(int)i + 42}));
  }
  return math::Vector<FF>(shares);
}

}  // namespace

TEST_CASE("Shamir reconstruct different x and alphas", "[ss]") {
  auto prg = util::PRG::create("shamir detect2");

  const auto shares = shareWithDifferentAlphas(prg, 3, 7);
  const auto alphas = math::Vector<FF>::range(42, 50);

  REQUIRE(ss::shamirRecoverD(shares, alphas, 3, 3, FF(0)) == FF(123));

  REQUIRE(ss::shamirRecoverD(shares, alphas, 3, 3, alphas[0]) == shares[0]);
}

TEST_CASE("Shamir reconstruct correct", "[sim]") {
  auto prg = util::PRG::create("shamir correct");
  auto shares = ss::shamirSecretShare(FF(123), 2, 7, prg);

  REQUIRE(ss::shamirRecoverC(shares).f.evaluate(FF{0}) == FF(123));

  shares[0] = FF(22);
  shares[1] = FF(23);

  REQUIRE(ss::shamirRecoverC(shares).f.evaluate(FF{0}) == FF(123));

  shares[2] = FF(24);

  REQUIRE_THROWS_MATCHES(ss::shamirRecoverC(shares),
                         std::logic_error,
                         Catch::Matchers::Message("could not correct shares"));
}

TEST_CASE("Shamir reconstruct correct different alphas", "[ss]") {
  auto prg = util::PRG::create("shamir correct2");

  auto shares = shareWithDifferentAlphas(prg, 2, 7);
  const auto alphas = math::Vector<FF>::range(42, 50);

  REQUIRE(ss::shamirRecoverC(shares, alphas).f.constantTerm() == FF(123));

  shares[4] = FF(5555);

  const auto r = ss::shamirRecoverC(shares, alphas);
  REQUIRE(r.f.constantTerm() == FF(123));
  REQUIRE(r.err.evaluate(alphas[4]) == FF(0));
}

TEST_CASE("BerlekampWelch wiki reference test", "[ss][math]") {
  // https://en.wikipedia.org/wiki/Berlekamp%E2%80%93Welch_algorithm#Example

  using FF = math::FF<test::GaloisField7>;

  math::Vector bs = {FF(1), FF(5), FF(3), FF(6), FF(3), FF(2), FF(2)};
  math::Vector corrected = {FF(1), FF(6), FF(3), FF(6), FF(1), FF(2), FF(2)};

  auto s = ss::shamirRecoverC(bs);
  // errors
  REQUIRE(s.err.evaluate(FF(2)) == FF::zero());
  REQUIRE(s.err.evaluate(FF(5)) == FF::zero());

  for (std::size_t i = 0; i < bs.size(); ++i) {
    REQUIRE(s.f.evaluate(FF(i + 1)) == corrected[i]);
  }
}
