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

#include "../gf7.h"
#include "scl/math/fp.h"
#include "scl/math/matrix.h"

using namespace scl;

using FF = math::FF<test::GaloisField7>;
using Matrix = math::Matrix<FF>;
using Vector = math::Vector<FF>;

const auto zero = FF::zero();
const auto one = FF::one();

TEST_CASE("LinAlg GetPivot", "[math][la]") {
  // [1 0 1]
  // [0 1 0]
  // [0 0 0]
  // clang-format off
  Matrix A = Matrix::fromVector(3, 3,
                                {one, zero, one,
                                 zero, one, zero,
                                 zero, zero, zero});
  // clang-format on
  REQUIRE(math::getPivotInColumn(A, 2) == -1);
  REQUIRE(math::getPivotInColumn(A, 1) == 1);
  REQUIRE(math::getPivotInColumn(A, 0) == 0);
  A(2, 2) = one;
  REQUIRE(math::getPivotInColumn(A, 2) == 2);

  Matrix B(2, 2);
  REQUIRE(math::getPivotInColumn(B, 0) == -1);
}

TEST_CASE("LinAlg FindFirstNonZeroRow", "[math][la]") {
  Matrix A =
      Matrix::fromVector(3,
                         3,
                         {one, zero, one, zero, one, zero, zero, zero, zero});
  REQUIRE(math::findFirstNonZeroRow(A) == 1);
  A(2, 1) = one;
  REQUIRE(math::findFirstNonZeroRow(A) == 2);
}

TEST_CASE("LinAlg ExtractSolution", "[math][la]") {
  // [1 0 0 3]
  // [0 1 0 5]
  // [0 0 1 2]
  // clang-format off
  Matrix A = Matrix::fromVector(3, 4,
                               {one, zero, zero, FF(3),
                                zero, one, zero, FF(5),
                                zero, zero, one, FF(2)}
    );
  // clang-format-on
  auto x = math::extractSolution(A);
  REQUIRE(x.equals(Vector{FF(3), FF(5), FF(2)}));

  // [1 3 1 2]
  // [0 0 1 4]
  // [0 0 0 0]
  // clang-format off
  Matrix B = Matrix::fromVector(3, 4,
                                {FF(1), FF(3), FF(1), FF(2),
                                 FF(0), FF(0), FF(1), FF(4),
                                 FF(0), FF(0), FF(0), FF(0)});
  // clang-format on
  auto y = math::extractSolution(B);
  REQUIRE(y.equals(Vector{FF(4), FF(4), FF(0)}));

  // [0 0 0 0]
  // [2 0 0 0]
  // [0 0 0 0]
  Matrix C(3, 4);
  C(1, 0) = FF(2);
  auto z = math::extractSolution(C);
  REQUIRE(z.equals(Vector{zero, one, zero}));
}

TEST_CASE("LinAlg Solve random", "[math][la]") {
  auto n = 10;
  auto prg = util::PRG::create();

  Matrix A = Matrix::random(n, n, prg);
  Vector b = Vector::random(n, prg);
  Vector x(n);
  math::solveLinearSystem(x, A, b);

  REQUIRE(A.multiply(x.toColumnMatrix()).equals(b.toColumnMatrix()));
}

TEST_CASE("LinAlg malformed systems", "[math][la]") {
  Vector x;
  Matrix A(2, 2);
  Vector b(3);
  REQUIRE_THROWS_MATCHES(
      math::solveLinearSystem(x, A, b),
      std::invalid_argument,
      Catch::Matchers::Message("malformed system of equations"));
}

TEST_CASE("LinAlg HasSolution", "[math][la]") {
  Matrix A(2, 3);
  // Has an all zero row, so no unique solution is possible
  REQUIRE_FALSE(math::hasSolution(A, true));
  // An all zero row implies a free variable, so many solutions exist
  REQUIRE(math::hasSolution(A, false));

  A(0, 2) = FF(1);
  REQUIRE_FALSE(math::hasSolution(A, false));
}
