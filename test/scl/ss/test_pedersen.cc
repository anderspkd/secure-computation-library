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

#include "scl/math/curves/secp256k1.h"
#include "scl/math/ec.h"
#include "scl/math/vector.h"
#include "scl/ss/pedersen.h"
#include "scl/ss/shamir.h"
#include "scl/util/prg.h"

using namespace scl;

using EC = math::EC<math::ec::Secp256k1>;
using FF = EC::ScalarField;

const EC h = EC::generator() * FF(42);

TEST_CASE("Pedersen", "[ss]") {
  auto prg = util::PRG::create("Pedersen");
  std::size_t t = 4;

  auto rand = FF(42);
  auto secret = FF(123);
  auto sb = ss::pedersenSecretShare<EC>(secret, t, 24, prg, h, rand);

  REQUIRE(sb.shares.size() == 24);
  REQUIRE(sb.commitments.size() == t + 1);
  REQUIRE(sb.commitments[0] == secret * EC::generator() + rand * h);

  auto sh = ss::shamirRecoverP(sb.shares.subVector(t + 1));
  REQUIRE(sh[0] == secret);
  REQUIRE(sh[1] == rand);
  REQUIRE(ss::pedersenVerify<EC>({sh, sb.commitments}, 0, h));
  // test overload
  REQUIRE(ss::pedersenVerify<EC>(sh, sb.commitments, 0, h));
}

TEST_CASE("Pedersen hom", "[ss]") {
  auto prg = util::PRG::create("Pedersen hom");
  std::size_t t = 4;

  auto s0 = FF(123);
  auto s1 = FF(44);

  auto ss0 = ss::pedersenSecretShare<EC>(s0, t, 10, prg, h);
  auto ss1 = ss::pedersenSecretShare<EC>(s1, t, 10, prg, h);

  auto ss2 = ss0.shares.add(ss1.shares);
  auto com2 = ss0.commitments.add(ss1.commitments);

  REQUIRE(ss::pedersenVerify<EC>({ss2[4], com2}, 5, h));

  auto secret = ss::shamirRecoverP(ss2.subVector(t + 1));
  // the recovered value is a pair {secret, randomness}.
  REQUIRE(secret[0] == s0 + s1);
  REQUIRE(ss::pedersenVerify<EC>({secret, com2}, 0, h));
}

namespace {

std::vector<std::vector<ss::PedersenShare<EC>>> getShares(std::size_t n,
                                                          std::size_t t) {
  auto prg = util::PRG::create("Pedersen apply");

  std::vector<std::vector<ss::PedersenShare<EC>>> shares(n);

  for (std::size_t i = 0; i < n; i++) {
    auto secret = FF::random(prg);
    auto shrs = ss::pedersenSecretShare<EC>(secret, t, n, prg, h);
    for (std::size_t j = 0; j < n; j++) {
      shares[j].emplace_back(shrs.getShare(j));
    }
  }

  return shares;
}

}  // namespace

TEST_CASE("Pedersen apply id", "[ss]") {
  const std::size_t t = 2;
  const std::size_t n = 5;

  auto shares_in = getShares(n, t);
  std::vector<std::vector<ss::PedersenShare<EC>>> shares_out;
  const auto id = math::Matrix<FF>::identity(n);

  for (std::size_t i = 0; i < n; i++) {
    const auto sin = shares_in[i];
    const auto sout = ss::apply<EC>(sin.begin(), sin.end(), id);
    for (std::size_t j = 0; j < n; j++) {
      REQUIRE(shares_in[i][j].share == sout[j].share);
      REQUIRE(shares_in[i][j].commitments == sout[j].commitments);
    }
  }
}

TEST_CASE("Pedersen apply", "[ss]") {
  const std::size_t t = 2;
  const std::size_t n = 5;

  auto shares_in = getShares(n, t);

  std::vector<std::vector<ss::PedersenShare<EC>>> shares_out;
  const auto van = math::Matrix<FF>::vandermonde(n - t, n);

  for (std::size_t i = 0; i < n; i++) {
    const auto sin = shares_in[i];
    shares_out.emplace_back(ss::apply(sin, van));
    REQUIRE(shares_out[i].size() == n - t);
  }

  // verify result
  for (std::size_t i = 0; i < n - t; i++) {
    for (std::size_t j = 0; j < n; j++) {
      const auto sij = shares_out[j][i];
      REQUIRE(ss::pedersenVerify(sij, j + 1, h));
    }
  }
}
