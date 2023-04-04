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
#include <stdexcept>

#include "../gf7.h"
#include "scl/math/fp.h"
#include "scl/math/lagrange.h"
#include "scl/math/vec.h"
#include "scl/ss/shamir.h"
#include "scl/util/prg.h"

using namespace scl;

using FF = math::Fp<61>;

TEST_CASE("Shamir share passive", "[ss]") {
  auto prg = util::PRG::Create("shamir passive");
  const auto shares = ss::ShamirShare(FF(123), 3, 4, prg);

  REQUIRE(shares.Size() == 4);
  REQUIRE(ss::ShamirRecoverP(shares) == FF(123));
}

TEST_CASE("Shamir reconstruct", "[ss]") {
  auto prg = util::PRG::Create("shamir recons");
  const auto shares = ss::ShamirShare(FF(123), 5, 100, prg);

  REQUIRE(shares.Size() == 100);

  const auto lb_0 =
      math::ComputeLagrangeBasis<FF>({FF(4), FF(5), FF(6), FF(7), FF(8), FF(9)},
                                     0);
  const auto r_0 = math::UncheckedInnerProd<FF>(shares.begin() + 3,
                                                shares.begin() + 9,
                                                lb_0.begin());
  const auto r_0_alt = shares.SubVector(3, 9).Dot(lb_0);

  REQUIRE(r_0 == FF(123));
  REQUIRE(r_0_alt == r_0);

  const auto lb_27 =
      math::ComputeLagrangeBasis<FF>({FF(4), FF(5), FF(6), FF(7), FF(8), FF(9)},
                                     27);

  const auto r_27 = math::UncheckedInnerProd<FF>(shares.begin() + 3,
                                                 shares.begin() + 9,
                                                 lb_27.begin());
  REQUIRE(r_27 == shares[26]);
}

TEST_CASE("Shamir reconstruct detect", "[ss]") {
  auto prg = util::PRG::Create("shamir detect");
  auto shares = ss::ShamirShare(FF(123), 4, 9, prg);

  REQUIRE(ss::ShamirRecoverD(shares) == FF(123));

  shares[2] = FF(4);
  REQUIRE_THROWS_MATCHES(
      ss::ShamirRecoverD(shares),
      std::logic_error,
      Catch::Matchers::Message("error detected during recovery"));
}

TEST_CASE("Shamir reconstruct correct", "[sim]") {
  auto prg = util::PRG::Create("shamir correct");
  auto shares = ss::ShamirShare(FF(123), 2, 7, prg);

  REQUIRE(ss::ShamirRecoverC(shares).f.Evaluate(FF{0}) == FF(123));

  shares[0] = FF(22);
  shares[1] = FF(23);

  REQUIRE(ss::ShamirRecoverC(shares).f.Evaluate(FF{0}) == FF(123));

  shares[2] = FF(24);

  REQUIRE_THROWS_MATCHES(ss::ShamirRecoverC(shares),
                         std::logic_error,
                         Catch::Matchers::Message("could not correct shares"));
}

TEST_CASE("BerlekampWelch", "[ss][math]") {
  // https://en.wikipedia.org/wiki/Berlekamp%E2%80%93Welch_algorithm#Example

  using FF = math::FF<test::GaloisField7>;

  math::Vec bs = {FF(1), FF(5), FF(3), FF(6), FF(3), FF(2), FF(2)};
  math::Vec corrected = {FF(1), FF(6), FF(3), FF(6), FF(1), FF(2), FF(2)};

  auto s = ss::ShamirRecoverC(bs);
  // errors
  REQUIRE(s.err.Evaluate(FF(2)) == FF::Zero());
  REQUIRE(s.err.Evaluate(FF(5)) == FF::Zero());

  for (std::size_t i = 0; i < bs.Size(); ++i) {
    REQUIRE(s.f.Evaluate(FF(i + 1)) == corrected[i]);
  }
}
