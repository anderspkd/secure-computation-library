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

#include "scl/math/fp.h"
#include "scl/math/matrix.h"

using namespace scl;

using FF = math::Fp<61>;
using Matrix = math::Matrix<FF>;
using Vector = math::Vector<FF>;

namespace {

void populate(Matrix& m, const std::vector<int>& values) {
  for (std::size_t i = 0; i < m.rows(); i++) {
    for (std::size_t j = 0; j < m.cols(); j++) {
      m(i, j) = FF(values[i * m.cols() + j]);
    }
  }
}

}  // namespace

TEST_CASE("Matrix construction", "[math][matrix]") {
  Matrix m0(2, 2);
  populate(m0, {1, 2, 5, 6});
  Matrix m1(2, 2);
  populate(m1, {4, 3, 2, 1});

  REQUIRE(!m0.equals(m1));
  REQUIRE(m0.rows() == 2);
  REQUIRE(m0.cols() == 2);
  REQUIRE(m0(0, 0) == FF(1));
  REQUIRE(m0(0, 1) == FF(2));
  REQUIRE(m0(1, 0) == FF(5));
  REQUIRE(m0(1, 1) == FF(6));

  Matrix a(5, 5);
  Matrix b(5);  // square matrix
  // matrices are 0 initialized, so the above matrices are equal
  REQUIRE(a.equals(b));

  REQUIRE_THROWS_MATCHES(Matrix(0, 1),
                         std::invalid_argument,
                         Catch::Matchers::Message("n or m cannot be 0"));
  REQUIRE_THROWS_MATCHES(Matrix(1, 0),
                         std::invalid_argument,
                         Catch::Matchers::Message("n or m cannot be 0"));
}

TEST_CASE("Matrix construction random", "[math][matrix]") {
  auto prg = util::PRG::create();
  Matrix mr = Matrix::random(4, 5, prg);
  REQUIRE(mr.rows() == 4);
  REQUIRE(mr.cols() == 5);
  bool not_zero = true;
  for (std::size_t i = 0; i < 4; i++) {
    for (std::size_t j = 0; j < 5; j++) {
      not_zero &= mr(i, j) != FF(0);
    }
  }
  REQUIRE(not_zero);
}

TEST_CASE("Matrix construction from Vec", "[math][matrix]") {
  Matrix m = Matrix::fromVector(2, 2, {FF(1), FF(2), FF(3), FF(4)});
  REQUIRE(m.rows() == 2);
  REQUIRE(m.cols() == 2);
  std::size_t k = 1;
  for (std::size_t i = 0; i < 2; ++i) {
    for (std::size_t j = 0; j < 2; ++j) {
      REQUIRE(m(i, j) == FF(k++));
    }
  }

  REQUIRE_THROWS_MATCHES(Matrix::fromVector(2, 2, {FF(1)}),
                         std::invalid_argument,
                         Catch::Matchers::Message("invalid dimensions"));
}

TEST_CASE("Matrix mutation", "[math][matrix]") {
  Matrix m0(2, 2);
  populate(m0, {1, 2, 5, 6});

  auto m = m0;
  m(0, 1) = FF(100);
  REQUIRE(m(0, 1) == FF(100));
  REQUIRE(m0(0, 1) == FF(2));
}

TEST_CASE("Matrix ToString", "[math][matrix]") {
  Matrix m(3, 2);
  populate(m, {1, 2, 44444, 5, 6, 7});
  std::string expected =
      "\n"
      "[    1  2 ]\n"
      "[ ad9c  5 ]\n"
      "[    6  7 ]";
  REQUIRE(m.toString() == expected);

  std::stringstream ss;
  ss << m;
  REQUIRE(ss.str() == expected);

  Matrix m1;
  REQUIRE(m1.toString() == "[ EMPTY MATRIX ]");
}

TEST_CASE("Matrix Addition", "[math][matrix]") {
  Matrix m0(2, 2);
  populate(m0, {1, 2, 5, 6});
  Matrix m1(2, 2);
  populate(m1, {4, 3, 2, 1});

  auto m2 = m0.add(m1);
  REQUIRE(m2.rows() == 2);
  REQUIRE(m2.cols() == 2);
  REQUIRE(m2(0, 0) == FF(5));
  REQUIRE(m2(0, 1) == FF(5));
  REQUIRE(m2(1, 0) == FF(7));
  REQUIRE(m2(1, 1) == FF(7));
  m2.addInPlace(m0);
  REQUIRE(m2.equals(m0.add(m0).add(m1)));
}

TEST_CASE("Matrix Subtraction", "[math][matrix]") {
  Matrix m0(2, 2);
  populate(m0, {1, 2, 5, 6});
  Matrix m1(2, 2);
  populate(m1, {4, 3, 2, 1});

  auto m2 = m0.subtract(m1);
  REQUIRE(m2(0, 0) == FF(1) - FF(4));
  REQUIRE(m2(0, 1) == FF(2) - FF(3));
  REQUIRE(m2(1, 0) == FF(3));
  REQUIRE(m2(1, 1) == FF(5));
  m2.subtractInPlace(m0);
  REQUIRE(m2.equals(m0.subtract(m0).subtract(m1)));
}

TEST_CASE("Matrix MultiplyEntryWise", "[math][matrix]") {
  Matrix m0(2, 2);
  populate(m0, {1, 2, 5, 6});
  Matrix m1(2, 2);
  populate(m1, {4, 3, 2, 1});

  auto m2 = m0.multiplyEntryWise(m1);
  REQUIRE(m2(0, 0) == FF(4));
  REQUIRE(m2(0, 1) == FF(6));
  REQUIRE(m2(1, 0) == FF(10));
  REQUIRE(m2(1, 1) == FF(6));
  m2.multiplyEntryWiseInPlace(m0);
  REQUIRE(m2.equals(m0.multiplyEntryWise(m0).multiplyEntryWise(m1)));
}

TEST_CASE("Matrix Multiply", "[math][matrix]") {
  Matrix m0(2, 2);
  populate(m0, {1, 2, 5, 6});
  Matrix m1(2, 2);
  populate(m1, {4, 3, 2, 1});

  auto m2 = m0.multiply(m1);
  REQUIRE(m2.rows() == 2);
  REQUIRE(m2.cols() == 2);
  REQUIRE(m2(0, 0) == FF(8));
  REQUIRE(m2(0, 1) == FF(5));
  REQUIRE(m2(1, 0) == FF(32));
  REQUIRE(m2(1, 1) == FF(21));

  Matrix m3(2, 10);
  populate(m3, {1,  2,  3,  4,  5,  6,  7,  8,  9,  0,
                11, 12, 13, 14, 15, 16, 17, 18, 19, 20});

  auto m5 = m0.multiply(m3);
  REQUIRE(m5.rows() == 2);
  REQUIRE(m5.cols() == 10);
  Matrix m4(2, 10);
  populate(m4, {23, 26, 29, 32,  35,  38,  41,  44,  47,  40,
                71, 82, 93, 104, 115, 126, 137, 148, 159, 120});
  REQUIRE(m5.equals(m4));

  REQUIRE_THROWS_MATCHES(
      m3.multiply(m0),
      std::invalid_argument,
      Catch::Matchers::Message("matmul: this->cols() != that->rows()"));
}

TEST_CASE("Matrix vector multiply", "[math][matrix]") {
  Matrix m0(2, 3);
  populate(m0, {1, 2, 3, 4, 5, 6});
  Vector v0 = {FF(1), FF(2), FF(3)};

  Vector v1 = m0.multiply(v0);
  REQUIRE(v1.size() == 2);
  REQUIRE(v1[0] == FF(1 * 1 + 2 * 2 + 3 * 3));
  REQUIRE(v1[1] == FF(4 * 1 + 5 * 2 + 6 * 3));

  Vector v2 = {FF(6), FF(7)};
  REQUIRE_THROWS_MATCHES(
      m0.multiply(v2),
      std::invalid_argument,
      Catch::Matchers::Message("matmul: this->cols() != vec.size()"));
}

TEST_CASE("Matrix ScalarMultiply", "[math][matrix]") {
  Matrix m0(2, 2);
  populate(m0, {1, 2, 5, 6});
  Matrix m1(2, 2);
  populate(m1, {4, 3, 2, 1});

  auto m2 = m0.scalarMultiply(FF(2));
  REQUIRE(m2(0, 0) == FF(2));
  REQUIRE(m2(0, 1) == FF(4));
  REQUIRE(m2(1, 0) == FF(10));
  REQUIRE(m2(1, 1) == FF(12));

  m2.scalarMultiplyInPlace(FF(2));
  REQUIRE(m2(0, 0) == FF(4));
  REQUIRE(m2(0, 1) == FF(8));
  REQUIRE(m2(1, 0) == FF(20));
  REQUIRE(m2(1, 1) == FF(24));
}

TEST_CASE("Matrix Transpose", "[math][matrix]") {
  Matrix m3(2, 3);
  populate(m3, {1, 2, 3, 11, 12, 13});
  auto m4 = m3.transpose();
  REQUIRE(m4.rows() == m3.cols());
  REQUIRE(m4.cols() == m3.rows());
  REQUIRE(m4(0, 0) == m3(0, 0));
  REQUIRE(m4(0, 1) == m3(1, 0));
  REQUIRE(m4(1, 0) == m3(0, 1));
  REQUIRE(m4(1, 1) == m3(1, 1));
  REQUIRE(m4(2, 0) == m3(0, 2));
  REQUIRE(m4(2, 1) == m3(1, 2));
}

TEST_CASE("Matrix check compatability", "[math][matrix]") {
  Matrix m0(2, 2);
  Matrix m1(3, 2);
  REQUIRE_THROWS_MATCHES(m1.add(m0),
                         std::invalid_argument,
                         Catch::Matchers::Message("incompatible matrices"));
}

TEST_CASE("Matrix resize", "[math][matrix]") {
  auto prg = util::PRG::create();
  Matrix m = Matrix::random(2, 4, prg);
  auto copy = m;
  copy.resize(1, 8);
  REQUIRE(copy.rows() == 1);
  REQUIRE(copy.cols() == 8);
  std::size_t c = 0;
  for (std::size_t i = 0; i < 2; ++i) {
    for (std::size_t j = 0; j < 4; ++j) {
      REQUIRE(m(i, j) == copy(0, c++));
    }
  }

  REQUIRE_THROWS_MATCHES(m.resize(42, 4),
                         std::invalid_argument,
                         Catch::Matchers::Message("cannot resize matrix"));
}

TEST_CASE("Matrix equality", "[math][matrix]") {
  auto prg = util::PRG::create();
  auto m0 = Matrix::random(3, 4, prg);
  auto m1 = Matrix::random(3, 4, prg);

  REQUIRE_FALSE(m0.equals(m1));
  prg.reset();
  m1 = Matrix::random(3, 4, prg);
  REQUIRE(m0.equals(m1));

  auto m2 = Matrix::random(2, 2, prg);
  REQUIRE_FALSE(m2.equals(m1));
}

TEST_CASE("Matrix isSquare", "[math][matrix]") {
  auto prg = util::PRG::create();
  Matrix sq = Matrix::random(2, 2, prg);
  REQUIRE(sq.isSquare());
  Matrix nsq = Matrix::random(4, 2, prg);
  REQUIRE(!nsq.isSquare());
}

TEST_CASE("Matrix identity", "[math][matrix]") {
  Matrix A = Matrix::identity(10);
  REQUIRE(A.rows() == 10);
  REQUIRE(A.cols() == 10);
  bool good = true;
  for (std::size_t i = 0; i < 10; ++i) {
    for (std::size_t j = 0; j < 10; ++j) {
      if (i == j) {
        good &= A(i, j) == FF(1);
      } else {
        good &= A(i, j) == FF(0);
      }
    }
  }
  REQUIRE(good);

  Matrix B(2, 1);
  REQUIRE_FALSE(B.isIdentity());
}

TEST_CASE("Matrix inversion", "[math][matrix]") {
  auto prg = util::PRG::create("mat_inv");
  const auto m = Matrix::random(10, 10, prg);
  const auto i = m.invert();
  REQUIRE(m.multiply(i).isIdentity());
}

TEST_CASE("Matrix inversion bad", "[math][matrix]") {
  auto prg = util::PRG::create("mat_inv2");
  auto m = Matrix::random(5, 6, prg);
  REQUIRE_THROWS_MATCHES(
      m.invert(),
      std::invalid_argument,
      Catch::Matchers::Message("cannot invert non-square matrix"));
}

TEST_CASE("Matrix poly eval and interp", "[math][matrix]") {
  auto prg = util::PRG::create("mat_interp");

  const std::size_t n = 10;
  const std::size_t t = 3;

  const math::Vector<FF> coeff = math::Vector<FF>::random(t, prg);

  const math::Matrix<FF> vand = math::Matrix<FF>::vandermonde(n, t);

  const auto evals = vand.multiply(coeff);

  REQUIRE(evals.size() == n);

  math::Vector<FF> points = {evals[0], evals[4], evals[5]};

  const math::Matrix<FF> vandi =
      math::Matrix<FF>::vandermonde(t,
                                    t,
                                    math::Vector<FF>{FF(1), FF(5), FF(6)});

  const auto coeff_ = vandi.invert().multiply(points);
  REQUIRE(coeff_ == coeff);
}

TEST_CASE("Matrix vandermonde", "[math][matrix]") {
  auto m0 = Matrix::vandermonde(3, 3);
  REQUIRE(m0(0, 0) == FF(1));
  REQUIRE(m0(0, 1) == FF(1));
  REQUIRE(m0(0, 2) == FF(1));
  REQUIRE(m0(1, 0) == FF(1));
  REQUIRE(m0(1, 1) == FF(2));
  REQUIRE(m0(1, 2) == FF(4));
  REQUIRE(m0(2, 0) == FF(1));
  REQUIRE(m0(2, 1) == FF(3));
  REQUIRE(m0(2, 2) == FF(9));

  std::vector<FF> xs{FF(3), FF(5), FF(8)};
  auto m1 = Matrix::vandermonde(3, 3, xs);
  REQUIRE(m1(0, 0) == FF(1));
  REQUIRE(m1(0, 1) == FF(3));
  REQUIRE(m1(0, 2) == FF(9));
  REQUIRE(m1(1, 0) == FF(1));
  REQUIRE(m1(1, 1) == FF(5));
  REQUIRE(m1(1, 2) == FF(25));
  REQUIRE(m1(2, 0) == FF(1));
  REQUIRE(m1(2, 1) == FF(8));
  REQUIRE(m1(2, 2) == FF(64));

  xs.emplace_back(FF(55));
  REQUIRE_THROWS_MATCHES(Matrix::vandermonde(3, 3, xs),
                         std::invalid_argument,
                         Catch::Matchers::Message("|xs| != number of rows"));
}

TEST_CASE("Matrix HIM", "[math][matrix]") {
  // TODO: Not a very good test.

  auto him = Matrix::hyperInvertible(4, 5);
  for (std::size_t i = 0; i < 4; ++i) {
    for (std::size_t j = 0; j < 5; ++j) {
      REQUIRE(him(i, j) != FF::zero());
    }
  }
}
