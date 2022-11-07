/**
 * @file test_shamir.cc
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

#include "../gf7.h"
#include "scl/math.h"
#include "scl/prg.h"
#include "scl/ss/shamir.h"

TEST_CASE("Shamir", "[ss]") {
  using FF = scl::Fp<61>;
  using Vec = scl::Vec<FF>;

  const std::size_t t = 2;

  SECTION("Reconstruct") {
    scl::PRG prg;
    scl::details::ShamirSSFactory<FF> factory(
        t, prg, scl::details::SecurityLevel::PASSIVE);
    auto intr = factory.GetInterpolator();

    auto secret = FF(123);

    auto shares = factory.Share(secret);
    auto s = intr.Reconstruct(shares);
    REQUIRE(s == secret);

    REQUIRE_THROWS_MATCHES(
        intr.Reconstruct(shares.SubVector(1)), std::invalid_argument,
        Catch::Matchers::Message("not enough shares to reconstruct"));
  }

  SECTION("Detection") {
    scl::PRG prg;
    scl::details::ShamirSSFactory<FF> factory(
        t, prg, scl::details::SecurityLevel::DETECT);
    auto intr = scl::details::Reconstructor<FF>::Create(
        t, scl::details::SecurityLevel::DETECT);

    auto secret = FF(555);
    auto shares = factory.Share(secret);
    REQUIRE(shares.Size() == 2 * t + 1);
    REQUIRE(intr.Reconstruct(shares) == secret);

    REQUIRE_THROWS_MATCHES(
        intr.Reconstruct(shares.SubVector(2)), std::invalid_argument,
        Catch::Matchers::Message("not enough shares to reconstruct"));

    auto ss = intr.ReconstructShare(shares, 2);
    REQUIRE(ss == shares[2]);
    REQUIRE(intr.Reconstruct(shares, 3) == intr.ReconstructShare(shares, 2));
  }

  SECTION("Robust") {
    scl::PRG prg;
    scl::details::ShamirSSFactory<FF> factory(
        t, prg, scl::details::SecurityLevel::CORRECT);

    // no errors
    auto secret = FF(123);
    auto shares = factory.Share(secret);
    REQUIRE(shares.Size() == 3 * t + 1);
    auto reconstructed = scl::details::ReconstructShamirRobust(shares, t);
    REQUIRE(reconstructed == secret);

    // can also reconstruct with an interpolator
    auto intr = scl::details::Reconstructor<FF>::Create(
        t, scl::details::SecurityLevel::CORRECT);
    REQUIRE(intr.Reconstruct(shares) == secret);

    // one error
    shares[0] = FF(63212);
    auto reconstructed_1 = scl::details::ReconstructShamirRobust(shares, t);
    REQUIRE(reconstructed_1 == secret);

    // two errors
    shares[2] = FF(63212211);
    auto reconstructed_2 = scl::details::ReconstructShamirRobust(shares, t);
    REQUIRE(reconstructed_2 == secret);

    // three errors -- that's one too many
    shares[1] = FF(123);
    REQUIRE_THROWS_MATCHES(
        scl::details::ReconstructShamirRobust(shares, t), std::logic_error,
        Catch::Matchers::Message("could not correct shares"));

    REQUIRE_THROWS_MATCHES(
        scl::details::ReconstructShamirRobust(shares, t + 1),
        std::invalid_argument,
        Catch::Matchers::Message(
            "not enough shares to reconstruct with error correction"));
    REQUIRE_THROWS_MATCHES(
        scl::details::ReconstructShamirRobust(shares, Vec{}, t),
        std::invalid_argument,
        Catch::Matchers::Message(
            "not enough alphas to reconstruct with error correction"));
  }
}

TEST_CASE("BerlekampWelch", "[ss][math]") {
  // https://en.wikipedia.org/wiki/Berlekamp%E2%80%93Welch_algorithm#Example

  using FF = scl::FF<scl::details::GF7>;
  using Vec = scl::Vec<FF>;

  Vec bs = {FF(1), FF(5), FF(3), FF(6), FF(3), FF(2), FF(2)};
  Vec as = {FF(0), FF(1), FF(2), FF(3), FF(4), FF(5), FF(6)};
  Vec corrected = {FF(1), FF(6), FF(3), FF(6), FF(1), FF(2), FF(2)};

  auto pe = scl::details::ReconstructShamirRobust(bs, as, 2);
  auto p = std::get<0>(pe);
  auto e = std::get<1>(pe);

  // errors
  REQUIRE(e.Evaluate(FF(1)) == FF{});
  REQUIRE(e.Evaluate(FF(4)) == FF{});

  for (std::size_t i = 0; i < as.Size(); ++i) {
    REQUIRE(p.Evaluate(as[i]) == corrected[i]);
  }
}
