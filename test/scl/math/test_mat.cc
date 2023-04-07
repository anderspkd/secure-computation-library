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
#include <iostream>
#include <stdexcept>

#include "scl/math/fp.h"
#include "scl/math/mat.h"

using namespace scl;

using FF = math::Fp<61>;
using Mat = math::Mat<FF>;
using Vec = math::Vec<FF>;

namespace {

void Populate(Mat& m, const std::vector<int>& values) {
  for (std::size_t i = 0; i < m.Rows(); i++) {
    for (std::size_t j = 0; j < m.Cols(); j++) {
      m(i, j) = FF(values[i * m.Cols() + j]);
    }
  }
}

}  // namespace

TEST_CASE("Matrix construction", "[math][matrix]") {
  Mat m0(2, 2);
  Populate(m0, {1, 2, 5, 6});
  Mat m1(2, 2);
  Populate(m1, {4, 3, 2, 1});

  REQUIRE(!m0.Equals(m1));
  REQUIRE(m0.Rows() == 2);
  REQUIRE(m0.Cols() == 2);
  REQUIRE(m0(0, 0) == FF(1));
  REQUIRE(m0(0, 1) == FF(2));
  REQUIRE(m0(1, 0) == FF(5));
  REQUIRE(m0(1, 1) == FF(6));

  Mat a(5, 5);
  Mat b(5);  // square matrix
  // matrices are 0 initialized, so the above matrices are equal
  REQUIRE(a.Equals(b));

  REQUIRE_THROWS_MATCHES(Mat(0, 1),
                         std::invalid_argument,
                         Catch::Matchers::Message("n or m cannot be 0"));
  REQUIRE_THROWS_MATCHES(Mat(1, 0),
                         std::invalid_argument,
                         Catch::Matchers::Message("n or m cannot be 0"));
}

TEST_CASE("Matrix construction random", "[math][matrix]") {
  auto prg = util::PRG::Create();
  Mat mr = Mat::Random(4, 5, prg);
  REQUIRE(mr.Rows() == 4);
  REQUIRE(mr.Cols() == 5);
  bool not_zero = true;
  for (std::size_t i = 0; i < 4; i++) {
    for (std::size_t j = 0; j < 5; j++) {
      not_zero &= mr(i, j) != FF(0);
    }
  }
  REQUIRE(not_zero);
}

TEST_CASE("Matrix construction from Vec", "[math][matrix]") {
  Mat m = Mat::FromVector(2, 2, {FF(1), FF(2), FF(3), FF(4)});
  REQUIRE(m.Rows() == 2);
  REQUIRE(m.Cols() == 2);
  std::size_t k = 1;
  for (std::size_t i = 0; i < 2; ++i) {
    for (std::size_t j = 0; j < 2; ++j) {
      REQUIRE(m(i, j) == FF(k++));
    }
  }

  REQUIRE_THROWS_MATCHES(Mat::FromVector(2, 2, {FF(1)}),
                         std::invalid_argument,
                         Catch::Matchers::Message("invalid dimensions"));
}

TEST_CASE("Matrix mutation", "[math][matrix]") {
  Mat m0(2, 2);
  Populate(m0, {1, 2, 5, 6});

  auto m = m0;
  m(0, 1) = FF(100);
  REQUIRE(m(0, 1) == FF(100));
  REQUIRE(m0(0, 1) == FF(2));
}

TEST_CASE("Matrix ToString", "[math][matrix]") {
  Mat m(3, 2);
  Populate(m, {1, 2, 44444, 5, 6, 7});
  std::string expected =
      "\n"
      "[    1  2 ]\n"
      "[ ad9c  5 ]\n"
      "[    6  7 ]";
  REQUIRE(m.ToString() == expected);

  std::stringstream ss;
  ss << m;
  REQUIRE(ss.str() == expected);

  Mat m1;
  REQUIRE(m1.ToString() == "[ EMPTY MATRIX ]");
}

TEST_CASE("Matrix Addition", "[math][matrix]") {
  Mat m0(2, 2);
  Populate(m0, {1, 2, 5, 6});
  Mat m1(2, 2);
  Populate(m1, {4, 3, 2, 1});

  auto m2 = m0.Add(m1);
  REQUIRE(m2.Rows() == 2);
  REQUIRE(m2.Cols() == 2);
  REQUIRE(m2(0, 0) == FF(5));
  REQUIRE(m2(0, 1) == FF(5));
  REQUIRE(m2(1, 0) == FF(7));
  REQUIRE(m2(1, 1) == FF(7));
  m2.AddInPlace(m0);
  REQUIRE(m2.Equals(m0.Add(m0).Add(m1)));
}

TEST_CASE("Matrix Subtraction", "[math][matrix]") {
  Mat m0(2, 2);
  Populate(m0, {1, 2, 5, 6});
  Mat m1(2, 2);
  Populate(m1, {4, 3, 2, 1});

  auto m2 = m0.Subtract(m1);
  REQUIRE(m2(0, 0) == FF(1) - FF(4));
  REQUIRE(m2(0, 1) == FF(2) - FF(3));
  REQUIRE(m2(1, 0) == FF(3));
  REQUIRE(m2(1, 1) == FF(5));
  m2.SubtractInPlace(m0);
  REQUIRE(m2.Equals(m0.Subtract(m0).Subtract(m1)));
}

TEST_CASE("Matrix MultiplyEntryWise", "[math][matrix]") {
  Mat m0(2, 2);
  Populate(m0, {1, 2, 5, 6});
  Mat m1(2, 2);
  Populate(m1, {4, 3, 2, 1});

  auto m2 = m0.MultiplyEntryWise(m1);
  REQUIRE(m2(0, 0) == FF(4));
  REQUIRE(m2(0, 1) == FF(6));
  REQUIRE(m2(1, 0) == FF(10));
  REQUIRE(m2(1, 1) == FF(6));
  m2.MultiplyEntryWiseInPlace(m0);
  REQUIRE(m2.Equals(m0.MultiplyEntryWise(m0).MultiplyEntryWise(m1)));
}

TEST_CASE("Matrix Multiply", "[math][matrix]") {
  Mat m0(2, 2);
  Populate(m0, {1, 2, 5, 6});
  Mat m1(2, 2);
  Populate(m1, {4, 3, 2, 1});

  auto m2 = m0.Multiply(m1);
  REQUIRE(m2.Rows() == 2);
  REQUIRE(m2.Cols() == 2);
  REQUIRE(m2(0, 0) == FF(8));
  REQUIRE(m2(0, 1) == FF(5));
  REQUIRE(m2(1, 0) == FF(32));
  REQUIRE(m2(1, 1) == FF(21));

  Mat m3(2, 10);
  Populate(m3, {1,  2,  3,  4,  5,  6,  7,  8,  9,  0,
                11, 12, 13, 14, 15, 16, 17, 18, 19, 20});

  auto m5 = m0.Multiply(m3);
  REQUIRE(m5.Rows() == 2);
  REQUIRE(m5.Cols() == 10);
  Mat m4(2, 10);
  Populate(m4, {23, 26, 29, 32,  35,  38,  41,  44,  47,  40,
                71, 82, 93, 104, 115, 126, 137, 148, 159, 120});
  REQUIRE(m5.Equals(m4));

  REQUIRE_THROWS_MATCHES(
      m3.Multiply(m0),
      std::invalid_argument,
      Catch::Matchers::Message("matmul: this->Cols() != that->Rows()"));
}

TEST_CASE("Matrix vector multiply", "[math][matrix]") {
  Mat m0(2, 3);
  Populate(m0, {1, 2, 3, 4, 5, 6});
  Vec v0 = {FF(1), FF(2), FF(3)};

  Vec v1 = m0.Multiply(v0);
  REQUIRE(v1.Size() == 2);
  REQUIRE(v1[0] == FF(1 * 1 + 2 * 2 + 3 * 3));
  REQUIRE(v1[1] == FF(4 * 1 + 5 * 2 + 6 * 3));

  Vec v2 = {FF(6), FF(7)};
  REQUIRE_THROWS_MATCHES(
      m0.Multiply(v2),
      std::invalid_argument,
      Catch::Matchers::Message("matmul: this->Cols() != vec.Size()"));
}

TEST_CASE("Matrix ScalarMultiply", "[math][matrix]") {
  Mat m0(2, 2);
  Populate(m0, {1, 2, 5, 6});
  Mat m1(2, 2);
  Populate(m1, {4, 3, 2, 1});

  auto m2 = m0.ScalarMultiply(FF(2));
  REQUIRE(m2(0, 0) == FF(2));
  REQUIRE(m2(0, 1) == FF(4));
  REQUIRE(m2(1, 0) == FF(10));
  REQUIRE(m2(1, 1) == FF(12));

  m2.ScalarMultiplyInPlace(FF(2));
  REQUIRE(m2(0, 0) == FF(4));
  REQUIRE(m2(0, 1) == FF(8));
  REQUIRE(m2(1, 0) == FF(20));
  REQUIRE(m2(1, 1) == FF(24));
}

TEST_CASE("Matrix Transpose", "[math][matrix]") {
  Mat m3(2, 3);
  Populate(m3, {1, 2, 3, 11, 12, 13});
  auto m4 = m3.Transpose();
  REQUIRE(m4.Rows() == m3.Cols());
  REQUIRE(m4.Cols() == m3.Rows());
  REQUIRE(m4(0, 0) == FF(1));
  REQUIRE(m4(0, 1) == FF(11));
  REQUIRE(m4(1, 0) == FF(2));
  REQUIRE(m4(1, 1) == FF(12));
  REQUIRE(m4(2, 0) == FF(3));
  REQUIRE(m4(2, 1) == FF(13));
}

TEST_CASE("Matrix check compatability", "[math][matrix]") {
  Mat m0(2, 2);
  Mat m1(3, 2);
  REQUIRE_THROWS_MATCHES(m1.Add(m0),
                         std::invalid_argument,
                         Catch::Matchers::Message("incompatible matrices"));
}

TEST_CASE("Matrix resize", "[math][matrix]") {
  auto prg = util::PRG::Create();
  Mat m = Mat::Random(2, 4, prg);
  auto copy = m;
  copy.Resize(1, 8);
  REQUIRE(copy.Rows() == 1);
  REQUIRE(copy.Cols() == 8);
  std::size_t c = 0;
  for (std::size_t i = 0; i < 2; ++i) {
    for (std::size_t j = 0; j < 4; ++j) {
      REQUIRE(m(i, j) == copy(0, c++));
    }
  }

  REQUIRE_THROWS_MATCHES(m.Resize(42, 4),
                         std::invalid_argument,
                         Catch::Matchers::Message("cannot resize matrix"));
}

TEST_CASE("Matrix equality", "[math][matrix]") {
  auto prg = util::PRG::Create();
  auto m0 = Mat::Random(3, 4, prg);
  auto m1 = Mat::Random(3, 4, prg);

  REQUIRE_FALSE(m0.Equals(m1));
  prg.Reset();
  m1 = Mat::Random(3, 4, prg);
  REQUIRE(m0.Equals(m1));

  auto m2 = Mat::Random(2, 2, prg);
  REQUIRE_FALSE(m2.Equals(m1));
}

TEST_CASE("Matrix isSquare", "[math][matrix]") {
  auto prg = util::PRG::Create();
  Mat sq = Mat::Random(2, 2, prg);
  REQUIRE(sq.IsSquare());
  Mat nsq = Mat::Random(4, 2, prg);
  REQUIRE(!nsq.IsSquare());
}

TEST_CASE("Matrix identity", "[math][matrix]") {
  Mat A = Mat::Identity(10);
  REQUIRE(A.Rows() == 10);
  REQUIRE(A.Cols() == 10);
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
}

TEST_CASE("Matrix vandermonde", "[math][matrix]") {
  auto m0 = Mat::Vandermonde(3, 3);
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
  auto m1 = Mat::Vandermonde(3, 3, xs);
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
  REQUIRE_THROWS_MATCHES(Mat::Vandermonde(3, 3, xs),
                         std::invalid_argument,
                         Catch::Matchers::Message("|xs| != number of rows"));
}

TEST_CASE("Matrix HIM", "[math][matrix]") {
  // TODO: Not a very good test.

  auto him = Mat::HyperInvertible(4, 5);
  for (std::size_t i = 0; i < 4; ++i) {
    for (std::size_t j = 0; j < 5; ++j) {
      REQUIRE(him(i, j) != FF::Zero());
    }
  }
}
