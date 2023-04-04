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

#include "../gf7.h"
#include "scl/math/fp.h"
#include "scl/math/la.h"
#include "scl/math/mat.h"

using namespace scl;

using FF = math::FF<test::GaloisField7>;
using Mat = math::Mat<FF>;
using Vec = math::Vec<FF>;

const auto zero = FF::Zero();
const auto one = FF::One();

TEST_CASE("LinAlg GetPivot", "[math][la]") {
  // [1 0 1]
  // [0 1 0]
  // [0 0 0]
  // clang-format off
  Mat A = Mat::FromVector(3, 3,
                          {one, zero, one,
                           zero, one, zero,
                           zero, zero, zero});
  // clang-format on
  REQUIRE(math::GetPivotInColumn(A, 2) == -1);
  REQUIRE(math::GetPivotInColumn(A, 1) == 1);
  REQUIRE(math::GetPivotInColumn(A, 0) == 0);
  A(2, 2) = one;
  REQUIRE(math::GetPivotInColumn(A, 2) == 2);

  Mat B(2, 2);
  REQUIRE(math::GetPivotInColumn(B, 0) == -1);
}

TEST_CASE("LinAlg FindFirstNonZeroRow", "[math][la]") {
  Mat A = Mat::FromVector(3,
                          3,
                          {one, zero, one, zero, one, zero, zero, zero, zero});
  REQUIRE(math::FindFirstNonZeroRow(A) == 1);
  A(2, 1) = one;
  REQUIRE(math::FindFirstNonZeroRow(A) == 2);
}

TEST_CASE("LinAlg ExtractSolution", "[math][la]") {
  // [1 0 0 3]
  // [0 1 0 5]
  // [0 0 1 2]
  // clang-format off
  Mat A = Mat::FromVector(3, 4,
                          {one, zero, zero, FF(3),
                           zero, one, zero, FF(5),
                           zero, zero, one, FF(2)}
    );
  // clang-format-on
  auto x = math::ExtractSolution(A);
  REQUIRE(x.Equals(Vec{FF(3), FF(5), FF(2)}));

  // [1 3 1 2]
  // [0 0 1 4]
  // [0 0 0 0]
  // clang-format off
  Mat B = Mat::FromVector(3, 4,
                          {FF(1), FF(3), FF(1), FF(2),
                           FF(0), FF(0), FF(1), FF(4),
                           FF(0), FF(0), FF(0), FF(0)});
  // clang-format on
  auto y = math::ExtractSolution(B);
  REQUIRE(y.Equals(Vec{FF(4), FF(4), FF(0)}));

  // [0 0 0 0]
  // [2 0 0 0]
  // [0 0 0 0]
  Mat C(3, 4);
  C(1, 0) = FF(2);
  auto z = math::ExtractSolution(C);
  REQUIRE(z.Equals(Vec{zero, one, zero}));
}

TEST_CASE("LinAlg Solve random", "[math][la]") {
  auto n = 10;
  auto prg = util::PRG::Create();

  Mat A = Mat::Random(n, n, prg);
  Vec b = Vec::Random(n, prg);
  Vec x(n);
  math::SolveLinearSystem(x, A, b);

  REQUIRE(A.Multiply(x.ToColumnMatrix()).Equals(b.ToColumnMatrix()));
}

TEST_CASE("LinAlg malformed systems", "[math][la]") {
  Vec x;
  Mat A(2, 2);
  Vec b(3);
  REQUIRE_THROWS_MATCHES(
      math::SolveLinearSystem(x, A, b),
      std::invalid_argument,
      Catch::Matchers::Message("malformed system of equations"));
}

TEST_CASE("LinAlg HasSolution", "[math][la]") {
  Mat A(2, 3);
  // Has an all zero row, so no unique solution is possible
  REQUIRE_FALSE(math::HasSolution(A, true));
  // An all zero row implies a free variable, so many solutions exist
  REQUIRE(math::HasSolution(A, false));

  A(0, 2) = FF(1);
  REQUIRE_FALSE(math::HasSolution(A, false));
}

TEST_CASE("LinAlg compute inverse", "[math][la]") {
  // TODO: This could/should be placed as a helper in the mat class, I think.

  std::size_t n = 10;
  auto prg = util::PRG::Create();
  Mat A = Mat::Random(n, n, prg);
  Mat I = Mat::Identity(n);

  auto aug = math::CreateAugmentedMatrix(A, I);
  REQUIRE_FALSE(aug.IsIdentity());
  math::RowReduceInPlace(aug);

  Mat Ainv(n, n);
  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t j = 0; j < n; ++j) {
      Ainv(i, j) = aug(i, n + j);
    }
  }

  REQUIRE_FALSE(A.IsIdentity());
  REQUIRE(A.Multiply(Ainv).IsIdentity());
}
