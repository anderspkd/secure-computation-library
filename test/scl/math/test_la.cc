/**
 * @file test_la.cc
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

#include "../gf7.h"
#include "scl/math/ff.h"
#include "scl/math/la.h"
#include "scl/math/mat.h"

using F = scl::details::FF<0, scl::details::GF7>;
using Mat = scl::Mat<F>;
using Vec = scl::Vec<F>;

TEST_CASE("LinearAlgebra", "[math]") {
  F zero;
  F one{1};

  SECTION("GetPivot") {
    // [1 0 1]
    // [0 1 0]
    // [0 0 0]
    Mat A = Mat::FromVector(3, 3,
                            {one, zero, one,   //
                             zero, one, zero,  //
                             zero, zero, zero});
    REQUIRE(scl::details::GetPivotInColumn(A, 2) == -1);
    REQUIRE(scl::details::GetPivotInColumn(A, 1) == 1);
    REQUIRE(scl::details::GetPivotInColumn(A, 0) == 0);
    A(2, 2) = one;
    REQUIRE(scl::details::GetPivotInColumn(A, 2) == 2);
  }

  SECTION("FindFirstNonZeroRow") {
    Mat A = Mat::FromVector(3, 3,
                            {one, zero, one,   //
                             zero, one, zero,  //
                             zero, zero, zero});
    REQUIRE(scl::details::FindFirstNonZeroRow(A) == 1);
    A(2, 1) = one;
    REQUIRE(scl::details::FindFirstNonZeroRow(A) == 2);
  }

  SECTION("ExtractSolution") {
    Mat A = Mat::FromVector(3, 4,
                            {one, zero, zero, F(3),  //
                             zero, one, zero, F(5),  //
                             zero, zero, one, F(2)});
    auto x = scl::details::ExtractSolution(A);
    REQUIRE(x.Equals(Vec{F(3), F(5), F(2)}));

    Mat B = Mat::FromVector(3, 4,
                            {F(1), F(3), F(1), F(2),  //
                             F(0), F(0), F(1), F(4),  //
                             F(0), F(0), F(0), F(0)});
    auto y = scl::details::ExtractSolution(B);
    REQUIRE(y.Equals(Vec{F(4), F(4), F(0)}));
  };

  SECTION("RandomSolve") {
    auto n = 10;
    scl::PRG prg;

    Mat A = Mat::Random(n, n, prg);
    Vec b = Vec::Random(n, prg);
    Vec x(n);
    scl::details::SolveLinearSystem(x, A, b);

    REQUIRE(A.Multiply(x.ToColumnMatrix()).Equals(b.ToColumnMatrix()));
  }

  SECTION("Inverse") {
    std::size_t n = 10;
    scl::PRG prg;
    Mat A = Mat::Random(n, n, prg);
    Mat I = Mat::Identity(n);

    auto aug = scl::details::CreateAugmentedMatrix(A, I);
    REQUIRE(!aug.IsIdentity());
    scl::details::RowReduceInPlace(aug);

    Mat Ainv(n, n);
    for (std::size_t i = 0; i < n; ++i) {
      for (std::size_t j = 0; j < n; ++j) {
        Ainv(i, j) = aug(i, n + j);
      }
    }

    REQUIRE(!A.IsIdentity());
    REQUIRE(A.Multiply(Ainv).IsIdentity());
  }
}
