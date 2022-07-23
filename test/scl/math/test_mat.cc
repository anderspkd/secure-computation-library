/**
 * @file test_mat.cc
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

#include "scl/math/fp.h"
#include "scl/math/mat.h"

using F = scl::Fp<61>;
using Mat = scl::Mat<F>;

inline void Populate(Mat& m, unsigned values[]) {
  for (std::size_t i = 0; i < m.Rows(); i++) {
    for (std::size_t j = 0; j < m.Cols(); j++) {
      m(i, j) = F(values[i * m.Cols() + j]);
    }
  }
}

TEST_CASE("Matrix", "[math]") {
  Mat m0(2, 2);
  unsigned v0[] = {1, 2, 5, 6};
  Populate(m0, v0);
  Mat m1(2, 2);
  unsigned v1[] = {4, 3, 2, 1};
  Populate(m1, v1);

  REQUIRE(!m0.Equals(m1));
  REQUIRE(m0.Rows() == 2);
  REQUIRE(m0.Cols() == 2);
  REQUIRE(m0(0, 0) == F(1));
  REQUIRE(m0(0, 1) == F(2));
  REQUIRE(m0(1, 0) == F(5));
  REQUIRE(m0(1, 1) == F(6));

  SECTION("mutate") {
    auto m = m0;
    m(0, 1) = F(100);
    REQUIRE(m(0, 1) == F(100));
    REQUIRE(m0(0, 1) == F(2));
  }

  SECTION("ToString") {
    Mat m(3, 2);
    unsigned v[] = {1, 2, 44444, 5, 6, 7};
    Populate(m, v);
    std::string expected =
        "\n"
        "[    1  2 ]\n"
        "[ ad9c  5 ]\n"
        "[    6  7 ]";
    REQUIRE(m.ToString() == expected);

    std::stringstream ss;
    ss << m;
    REQUIRE(ss.str() == expected);
  }

  SECTION("Construction") {
    Mat a(5, 5);
    Mat b(5);  // square matrix
    // matrices are 0 initialized, so the above matrices are equal
    REQUIRE(a.Equals(b));

    REQUIRE_THROWS_MATCHES(Mat(0, 1), std::invalid_argument,
                           Catch::Matchers::Message("n or m cannot be 0"));
    REQUIRE_THROWS_MATCHES(Mat(1, 0), std::invalid_argument,
                           Catch::Matchers::Message("n or m cannot be 0"));
  }

  SECTION("Add") {
    auto m2 = m0.Add(m1);
    REQUIRE(m2.Rows() == 2);
    REQUIRE(m2.Cols() == 2);
    REQUIRE(m2(0, 0) == F(5));
    REQUIRE(m2(0, 1) == F(5));
    REQUIRE(m2(1, 0) == F(7));
    REQUIRE(m2(1, 1) == F(7));
    m2.AddInPlace(m0);
    REQUIRE(m2.Equals(m0.Add(m0).Add(m1)));
  }

  SECTION("Subtract") {
    auto m2 = m0.Subtract(m1);
    REQUIRE(m2(0, 0) == F(1) - F(4));
    REQUIRE(m2(0, 1) == F(2) - F(3));
    REQUIRE(m2(1, 0) == F(3));
    REQUIRE(m2(1, 1) == F(5));
    m2.SubtractInPlace(m0);
    REQUIRE(m2.Equals(m0.Subtract(m0).Subtract(m1)));
  }

  SECTION("MultiplyEntryWise") {
    auto m2 = m0.MultiplyEntryWise(m1);
    REQUIRE(m2(0, 0) == F(4));
    REQUIRE(m2(0, 1) == F(6));
    REQUIRE(m2(1, 0) == F(10));
    REQUIRE(m2(1, 1) == F(6));
    m2.MultiplyEntryWiseInPlace(m0);
    REQUIRE(m2.Equals(m0.MultiplyEntryWise(m0).MultiplyEntryWise(m1)));
  }

  SECTION("Incompatible") {
    Mat m2(3, 2);
    REQUIRE_THROWS_MATCHES(m2.Add(m0), std::invalid_argument,
                           Catch::Matchers::Message("incompatible matrices"));
  }

  SECTION("Multiply") {
    auto m2 = m0.Multiply(m1);
    REQUIRE(m2.Rows() == 2);
    REQUIRE(m2.Cols() == 2);
    REQUIRE(m2(0, 0) == F(8));
    REQUIRE(m2(0, 1) == F(5));
    REQUIRE(m2(1, 0) == F(32));
    REQUIRE(m2(1, 1) == F(21));

    Mat m3(2, 10);
    unsigned v3[] = {1,  2,  3,  4,  5,  6,  7,  8,  9,  0,
                     11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
    Populate(m3, v3);

    auto m5 = m0.Multiply(m3);
    REQUIRE(m5.Rows() == 2);
    REQUIRE(m5.Cols() == 10);
    Mat m4(2, 10);
    unsigned v4[] = {23, 26, 29, 32,  35,  38,  41,  44,  47,  40,
                     71, 82, 93, 104, 115, 126, 137, 148, 159, 120};
    Populate(m4, v4);
    REQUIRE(m5.Equals(m4));

    REQUIRE_THROWS_MATCHES(
        m3.Multiply(m0), std::invalid_argument,
        Catch::Matchers::Message("invalid matrix dimensions for multiply"));
  }

  SECTION("ScalarMultiply") {
    auto m2 = m0.ScalarMultiply(F(2));
    REQUIRE(m2(0, 0) == F(2));
    REQUIRE(m2(0, 1) == F(4));
    REQUIRE(m2(1, 0) == F(10));
    REQUIRE(m2(1, 1) == F(12));

    m2.ScalarMultiplyInPlace(F(2));
    REQUIRE(m2(0, 0) == F(4));
    REQUIRE(m2(0, 1) == F(8));
    REQUIRE(m2(1, 0) == F(20));
    REQUIRE(m2(1, 1) == F(24));
  }

  SECTION("Transpose") {
    Mat m3(2, 3);
    unsigned v3[] = {1, 2, 3, 11, 12, 13};
    Populate(m3, v3);
    auto m4 = m3.Transpose();
    REQUIRE(m4.Rows() == m3.Cols());
    REQUIRE(m4.Cols() == m3.Rows());
    REQUIRE(m4(0, 0) == F(1));
    REQUIRE(m4(0, 1) == F(11));
    REQUIRE(m4(1, 0) == F(2));
    REQUIRE(m4(1, 1) == F(12));
    REQUIRE(m4(2, 0) == F(3));
    REQUIRE(m4(2, 1) == F(13));
  }

  SECTION("Random") {
    scl::PRG prg;
    Mat mr = Mat::Random(4, 5, prg);
    REQUIRE(mr.Rows() == 4);
    REQUIRE(mr.Cols() == 5);
    bool not_zero = true;
    for (std::size_t i = 0; i < 4; i++) {
      for (std::size_t j = 0; j < 5; j++) {
        not_zero &= mr(i, j) != F(0);
      }
    }
    REQUIRE(not_zero);

    // check stability
    scl::PRG prg1;
    Mat mr1 = Mat::Random(4, 5, prg1);
    REQUIRE(mr1.Equals(mr));
  }

  SECTION("fromVec") {
    Mat m = Mat::FromVector(2, 2, {F(1), F(2), F(3), F(4)});
    REQUIRE(m.Rows() == 2);
    REQUIRE(m.Cols() == 2);
    std::size_t k = 1;
    for (std::size_t i = 0; i < 2; ++i) {
      for (std::size_t j = 0; j < 2; ++j) {
        REQUIRE(m(i, j) == F(k++));
      }
    }
  }

  SECTION("resize") {
    scl::PRG prg;
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

    REQUIRE_THROWS_MATCHES(m.Resize(42, 4), std::invalid_argument,
                           Catch::Matchers::Message("cannot resize matrix"));
  }

  SECTION("IsSquare") {
    scl::PRG prg;
    Mat sq = Mat::Random(2, 2, prg);
    REQUIRE(sq.IsSquare());
    Mat nsq = Mat::Random(4, 2, prg);
    REQUIRE(!nsq.IsSquare());
  }

  SECTION("Identity") {
    Mat A = Mat::Identity(10);
    REQUIRE(A.Rows() == 10);
    REQUIRE(A.Cols() == 10);
    bool good = true;
    for (std::size_t i = 0; i < 10; ++i) {
      for (std::size_t j = 0; j < 10; ++j) {
        if (i == j)
          good &= A(i, j) == F(1);
        else
          good &= A(i, j) == F(0);
      }
    }
    REQUIRE(good);
  }

  SECTION("Vandermonde") {
    auto m0 = Mat::Vandermonde(3, 3);
    REQUIRE(m0(0, 0) == F(1));
    REQUIRE(m0(0, 1) == F(1));
    REQUIRE(m0(0, 2) == F(1));
    REQUIRE(m0(1, 0) == F(1));
    REQUIRE(m0(1, 1) == F(2));
    REQUIRE(m0(1, 2) == F(4));
    REQUIRE(m0(2, 0) == F(1));
    REQUIRE(m0(2, 1) == F(3));
    REQUIRE(m0(2, 2) == F(9));

    std::vector<F> xs{F(3), F(5), F(8)};
    auto m1 = Mat::Vandermonde(3, 3, xs);
    REQUIRE(m1(0, 0) == F(1));
    REQUIRE(m1(0, 1) == F(3));
    REQUIRE(m1(0, 2) == F(9));
    REQUIRE(m1(1, 0) == F(1));
    REQUIRE(m1(1, 1) == F(5));
    REQUIRE(m1(1, 2) == F(25));
    REQUIRE(m1(2, 0) == F(1));
    REQUIRE(m1(2, 1) == F(8));
    REQUIRE(m1(2, 2) == F(64));

    xs.emplace_back(F(55));
    REQUIRE_THROWS_MATCHES(Mat::Vandermonde(3, 3, xs), std::invalid_argument,
                           Catch::Matchers::Message("|xs| != number of rows"));
  }
}
