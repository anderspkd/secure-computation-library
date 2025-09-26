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

#include "../gf7.h"
#include "scl/math/ff.h"
#include "scl/math/matrix.h"
#include "scl/math/vector.h"

using namespace scl;

using Elem = FF<test::GaloisField7>;

const auto zero = Elem::zero();
const auto one = Elem::one();

TEST_CASE("LinAlg GetPivot", "[math][la]") {
  // [1 0 1]
  // [0 1 0]
  // [0 0 0]
  // clang-format off
  Matrix A = Matrix<Elem>::fromVector(3, 3,
                                {one, zero, one,
                                 zero, one, zero,
                                 zero, zero, zero});
  // clang-format on
  REQUIRE(getPivotInColumn(A, 2) == -1);
  REQUIRE(getPivotInColumn(A, 1) == 1);
  REQUIRE(getPivotInColumn(A, 0) == 0);
  A(2, 2) = one;
  REQUIRE(getPivotInColumn(A, 2) == 2);

  Matrix<Elem> B(2, 2);
  REQUIRE(getPivotInColumn(B, 0) == -1);
}

TEST_CASE("LinAlg FindFirstNonZeroRow", "[math][la]") {
  Matrix A = Matrix<Elem>::fromVector(
      3,
      3,
      {one, zero, one, zero, one, zero, zero, zero, zero});
  REQUIRE(findFirstNonZeroRow(A) == 1);
  A(2, 1) = one;
  REQUIRE(findFirstNonZeroRow(A) == 2);
}

TEST_CASE("LinAlg ExtractSolution", "[math][la]") {
  // [1 0 0 3]
  // [0 1 0 5]
  // [0 0 1 2]
  // clang-format off
  Matrix A = Matrix<Elem>::fromVector(3, 4,
                               {one, zero, zero, Elem(3),
                                zero, one, zero, Elem(5),
                                zero, zero, one, Elem(2)}
    );
  // clang-format-on
  auto x = extractSolution(A);
  REQUIRE(x.equals(Vector{Elem(3), Elem(5), Elem(2)}));

  // [1 3 1 2]
  // [0 0 1 4]
  // [0 0 0 0]
  // clang-format off
  Matrix B = Matrix<Elem>::fromVector(3, 4,
                                {Elem(1), Elem(3), Elem(1), Elem(2),
                                 Elem(0), Elem(0), Elem(1), Elem(4),
                                 Elem(0), Elem(0), Elem(0), Elem(0)});
  // clang-format on
  auto y = extractSolution(B);
  REQUIRE(y.equals(Vector{Elem(4), Elem(4), Elem(0)}));

  // [0 0 0 0]
  // [2 0 0 0]
  // [0 0 0 0]
  Matrix<Elem> C(3, 4);
  C(1, 0) = Elem(2);
  auto z = extractSolution(C);
  REQUIRE(z.equals(Vector{zero, one, zero}));
}

TEST_CASE("LinAlg Solve random", "[math][la]") {
  auto n = 10;
  auto prg = PRG::create();

  Matrix A = Matrix<Elem>::random(n, n, prg);
  Vector b = Vector<Elem>::random(n, prg);
  Vector<Elem> x(n);
  solveLinearSystem(x, A, b);

  REQUIRE(A.multiply(x.toColumnMatrix()).equals(b.toColumnMatrix()));
}

TEST_CASE("LinAlg malformed systems", "[math][la]") {
  Vector<Elem> x;
  Matrix<Elem> A(2, 2);
  Vector<Elem> b(3);
  REQUIRE_THROWS_MATCHES(
      solveLinearSystem(x, A, b),
      std::invalid_argument,
      Catch::Matchers::Message("malformed system of equations"));
}

TEST_CASE("LinAlg HasSolution", "[math][la]") {
  Matrix<Elem> A(2, 3);
  // Has an all zero row, so no unique solution is possible
  REQUIRE_FALSE(hasSolution(A, true));
  // An all zero row implies a free variable, so many solutions exist
  REQUIRE(hasSolution(A, false));

  A(0, 2) = Elem(1);
  REQUIRE_FALSE(hasSolution(A, false));
}
