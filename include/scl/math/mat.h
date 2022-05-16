/**
 * @file mat.h
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

#ifndef _SCL_MATH_MAT_H
#define _SCL_MATH_MAT_H

#include <cstdint>
#include <cstring>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "scl/prg.h"

namespace scl {

template <typename T>
class Vec;

/**
 * @brief Matrix.
 */
template <typename T>
class Mat {
 public:
  /**
   * @brief The type of the matrix elements.
   */
  using ValueType = T;

  /**
   * @brief Read a matrix from a stream of bytes.
   * @param n the number of rows
   * @param m the number of columns
   * @param src the bytes
   * @return a matrix.
   */
  static Mat<T> Read(std::size_t n, std::size_t m, const unsigned char* src);

  /**
   * @brief Create a Matrix and populate it with random elements.
   * @param n the number of rows
   * @param m the number of columns
   * @param prg the prg used to generate random elements
   * @return a Matrix with random elements.
   */
  static Mat<T> Random(std::size_t n, std::size_t m, PRG& prg);

  /**
   * @brief Create an N-by-M Vandermonde matrix.
   * @param n the number of rows
   * @param m the number of columns
   * @param xs a \p n length vector containing the x values to use
   * @return a Vandermonde matrix.
   */
  static Mat<T> Vandermonde(std::size_t n, std::size_t m,
                            const std::vector<T>& xs);

  /**
   * @brief Create an N-by-M Vandermonde matrix.
   * @param n the number of rows
   * @param m the number of columns
   * @return a Vandermonde matrix.
   */
  static Mat<T> Vandermonde(std::size_t n, std::size_t m) {
    std::vector<T> xs;
    xs.reserve(n);
    for (std::size_t i = 0; i < n; ++i) xs.emplace_back(T(i + 1));
    return Mat<T>::Vandermonde(n, m, xs);
  }

  /**
   * @brief Create an N-by-M hyper-invertible matrix.
   *
   * A hyper-invertible matrix is a matrix where every square sub-matrix is
   * invertible.
   *
   * @param n the number of rows
   * @param m the number of columns
   * @return a Hyperinvertible matrix.
   */
  static Mat<T> HyperInvertible(std::size_t n, std::size_t m);

  /**
   * @brief Create a matrix from a vector.
   *
   * The idea of this method is to allow one to write
   *
   *  <code>
   *  Mat::FromVector(2, 2, {1, 2, 3, 4})
   *  </code>
   *
   * in order to create the matrix [[1, 2], [3, 4]].
   *
   * @param n the number of rows
   * @param m the number of columns
   * @param vec the elements of the matrix
   * @return a Matrix.
   */
  static Mat<T> FromVector(std::size_t n, std::size_t m,
                           const std::vector<T>& vec) {
    if (vec.size() != n * m) throw std::invalid_argument("invalid dimensions");
    return Mat<T>(n, m, vec);
  };

  /**
   * @brief Construct an n-by-n identity matrix.
   */
  static Mat<T> Identity(std::size_t n) {
    Mat<T> I(n);
    for (std::size_t i = 0; i < n; ++i) I(i, i) = T(1);
    return I;
  }

  Mat() : mRows(0), mCols(0){};

  /**
   * @brief Create an N-by-M matrix with default initialized values.
   * @param n the number of rows
   * @param m the number of columns
   */
  explicit Mat(std::size_t n, std::size_t m) {
    if (!(n && m)) throw std::invalid_argument("n or m cannot be 0");
    std::vector<T> v(n * m);
    mRows = n;
    mCols = m;
    mValues = v;
  };

  /**
   * @brief Create a square matrix with default initialized values.
   * @param n the dimensions of the matrix
   */
  explicit Mat(std::size_t n) : Mat(n, n){};

  /**
   * @brief The number of rows of this matrix.
   */
  std::size_t Rows() const { return mRows; };

  /**
   * @brief The number of columns of this matrix.
   */
  std::size_t Cols() const { return mCols; };

  /**
   * @brief Provides mutable access to a matrix element.
   * @param row the row of the element being queried
   * @param column the column of the element being queried
   * @return an element of the matrix.
   */
  T& operator()(std::size_t row, std::size_t column) {
    return mValues[mCols * row + column];
  };

  /**
   * @brief Provides read-only access to a matrix element.
   * @param row the row of the element being queried
   * @param column the column of the element being queried
   * @return an element of the matrix.
   */
  T operator()(std::size_t row, std::size_t column) const {
    return mValues[mCols * row + column];
  };

  /**
   * @brief Add this matrix with another matrix of the same dimensions.
   * @param other the other matrix
   * @return The entry-wise sum of this matrix and \p other.
   * @throws std::illegal_argument if the dimensions of this and \p other are
   *         not equal.
   */
  Mat Add(const Mat& other) const {
    Mat copy(mRows, mCols, mValues);
    return copy.AddInPlace(other);
  };

  /**
   * @brief Add this matrix with another matrix of the same dimensions in-place.
   * @param other the other matrix
   * @return The entry-wise sum of this matrix and \p other.
   * @throws std::illegal_argument if the dimensions of this and \p other are
   *         not equal.
   */
  Mat& AddInPlace(const Mat& other) {
    EnsureCompatible(other);
    auto n = mValues.size();
    for (std::size_t i = 0; i < n; ++i) {
      mValues[i] += other.mValues[i];
    }
    return *this;
  };

  /**
   * @brief Subtract this matrix with another matrix of the same dimensions.
   * @param other the other matrix
   * @return the difference of this and \p other
   * @throws std::illegal_argument if the dimensions of this and \p do not
   *         match.
   */
  Mat Subtract(const Mat& other) const {
    Mat copy(mRows, mCols, mValues);
    return copy.SubtractInPlace(other);
  };

  /**
   * @brief Subtract this matrix with another matrix of the same dimensions
   *        in-place.
   * @param other the other matrix
   * @return the difference of this and \p other
   * @throws std::illegal_argument if the dimensions of this and \p do not
   *         match.
   */
  Mat& SubtractInPlace(const Mat& other) {
    EnsureCompatible(other);
    auto n = mValues.size();
    for (std::size_t i = 0; i < n; i++) {
      mValues[i] -= other.mValues[i];
    }
    return *this;
  };

  /**
   * @brief Multiply this matrix with another matrix of the same dimensions.
   * @param other the other matrix
   * @return the entry-wise product of this and \p other
   * @throws std::illegal_argument if the dimensions of this and \p do not
   *         match.
   */
  Mat MultiplyEntryWise(const Mat& other) const {
    Mat copy(mRows, mCols, mValues);
    return copy.MultiplyEntryWiseInPlace(other);
  };

  /**
   * @brief Multiply this matrix with another matrix of the same dimensions.
   * @param other the other matrix
   * @return the product of this and \p other
   * @throws std::illegal_argument if the dimensions of this and \p do not
   *         match.
   */
  Mat& MultiplyEntryWiseInPlace(const Mat& other) {
    EnsureCompatible(other);
    auto n = mValues.size();
    for (std::size_t i = 0; i < n; i++) {
      mValues[i] *= other.mValues[i];
    }
    return *this;
  };

  /**
   * @brief Performs a matrix multiplication.
   * @param other the other matrix
   * @return the matrix product of this and \p other.
   * @throws std::illegal_argument if the dimensions of the inputs are
   *         incompatible.
   */
  Mat Multiply(const Mat& other) const;

  /**
   * @brief Multiply this matrix with a scalar
   * @param scalar the scalar
   * @return this scaled by \p scalar.
   */
  Mat ScalarMultiply(const T& scalar) const {
    Mat copy(mRows, mCols, mValues);
    return copy.ScalarMultiplyInPlace(scalar);
  };

  /**
   * @brief Multiply this matrix with a scalar in-place.
   * @param scalar the scalar
   * @return this scaled by \p scalar.
   */
  Mat& ScalarMultiplyInPlace(const T& scalar) {
    for (auto& v : mValues) v *= scalar;
    return *this;
  };

  /**
   * @brief Check if this matrix is square.
   */
  bool IsSquare() const { return Rows() == Cols(); };

  /**
   * @brief Transpose this matrix.
   * @return the transpose of this.
   */
  Mat Transpose() const;

  /**
   * @brief Resize this matrix.
   * @param rows the new row count
   * @param cols the new column count
   */
  Mat& Resize(std::size_t rows, std::size_t cols) {
    if (rows * cols != Rows() * Cols())
      throw std::invalid_argument("Cannot resize matrix");
    mRows = rows;
    mCols = cols;
    return *this;
  };

  /**
   * @brief Returns true if this matrix is the identity matrix.
   */
  bool IsIdentity() const;

  /**
   * @brief Return a string representation of this matrix.
   */
  std::string ToString() const;

  /**
   * @brief Write a string representation of this vector to a stream.
   */
  friend std::ostream& operator<<(std::ostream& os, const Mat<T>& v) {
    return os << v.ToString();
  };

  /**
   * @brief Test if this matrix is equal to another.
   */
  bool Equals(const Mat& other) const {
    if (Rows() != other.Rows() || Cols() != other.Cols()) return false;

    bool equal = true;
    for (std::size_t i = 0; i < mValues.size(); i++)
      equal &= mValues[i] == other.mValues[i];
    return equal;
  };

  /**
   * @brief Write this matrix to a buffer.
   *
   * The matrix is written as
   *
   *   <code>row_count || column_count || data</code>
   *
   * where each of <code>row_count</code> and <code>column_count</code> are 4
   * bytes large. <code>data</code> is the content of the matrix, in row-major
   * order.
   *
   * @param dest where to write the matrix
   */
  void Write(unsigned char* dest) const;

  /**
   * @brief The size of a matrix when serialized in bytes.
   */
  std::size_t ByteSize() const { return Rows() * Cols() * T::ByteSize(); }

 private:
  Mat(std::size_t r, std::size_t c, std::vector<T> v)
      : mRows(r), mCols(c), mValues(v){};

  void EnsureCompatible(const Mat& other) {
    if (mRows != other.mRows || mCols != other.mCols)
      throw std::invalid_argument("incompatible matrices");
  };

  std::size_t mRows;
  std::size_t mCols;
  std::vector<T> mValues;

  friend class Vec<T>;
};

template <typename T>
Mat<T> Mat<T>::Read(std::size_t n, std::size_t m, const unsigned char* src) {
  auto ptr = src;
  auto total = n * m;

  // write all elements now that we know we'll not exceed the maximum read size.
  std::vector<T> elements;
  elements.reserve(total);
  for (std::size_t i = 0; i < total; i++) {
    elements.emplace_back(T::Read(ptr));
    ptr += T::ByteSize();
  }
  return Mat(n, m, elements);
}

template <typename T>
void Mat<T>::Write(unsigned char* dest) const {
  auto ptr = dest;
  for (const auto& v : mValues) {
    v.Write(ptr);
    ptr += T::ByteSize();
  }
}

template <typename T>
Mat<T> Mat<T>::Random(std::size_t n, std::size_t m, PRG& prg) {
  std::size_t nelements = n * m;
  std::vector<T> elements;
  elements.reserve(nelements);

  std::size_t buffer_size = nelements * T::ByteSize();
  auto buffer = std::make_unique<unsigned char[]>(buffer_size);
  auto ptr = buffer.get();
  prg.Next(buffer.get(), buffer_size);
  for (std::size_t i = 0; i < nelements; i++)
    elements.emplace_back(T::Read(ptr + i * T::ByteSize()));

  return Mat(n, m, elements);
}

template <typename T>
Mat<T> Mat<T>::Vandermonde(std::size_t n, std::size_t m,
                           const std::vector<T>& xs) {
  if (xs.size() != n) {
    throw std::invalid_argument("|xs| != number of rows");
  }

  Mat<T> v(n, m);
  for (std::size_t i = 0; i < n; ++i) {
    v(i, 0) = T(1);
    for (std::size_t j = 1; j < m; ++j) {
      v(i, j) = v(i, j - 1) * xs[i];
    }
  }
  return v;
}

namespace {

template <typename T>
T LagrangeCoefficient(std::size_t i, std::size_t j, std::size_t n) {
  const T b(i);
  const T a(j);
  T lm(1);
  for (std::size_t k = 0; k < n; ++k) {
    if (k == j) {
      lm *= (b - T(k)) / (a - T(k));
    }
  }
  return lm;
}

}  // namespace

template <typename T>
Mat<T> Mat<T>::HyperInvertible(std::size_t n, std::size_t m) {
  Mat<T> him(n, m);
  const auto mx = std::max(n, m);

  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t j = 0; j < m; ++j) {
      him(i, j) = LagrangeCoefficient<T>(i, j, mx);
    }
  }

  return him;
}

template <typename T>
Mat<T> Mat<T>::Multiply(const Mat<T>& other) const {
  if (Cols() != other.Rows())
    throw std::invalid_argument("invalid matrix dimensions for multiply");
  const auto n = Rows();
  const auto p = Cols();
  const auto m = other.Cols();

  Mat result(n, m);
  for (std::size_t i = 0; i < n; i++) {
    for (std::size_t k = 0; k < p; k++) {
      for (std::size_t j = 0; j < m; j++) {
        result(i, j) += operator()(i, k) * other(k, j);
      }
    }
  }
  return result;
}

template <typename T>
Mat<T> Mat<T>::Transpose() const {
  Mat t(Cols(), Rows());
  for (std::size_t i = 0; i < Rows(); i++) {
    for (std::size_t j = 0; j < Cols(); j++) {
      t(j, i) = operator()(i, j);
    }
  }
  return t;
}

template <typename T>
bool Mat<T>::IsIdentity() const {
  if (!IsSquare()) return false;

  bool is_ident = true;
  for (std::size_t i = 0; i < Rows(); ++i) {
    for (std::size_t j = 0; j < Cols(); ++j) {
      if (i == j)
        is_ident &= operator()(i, j) == T{1};
      else
        is_ident &= operator()(i, j) == T{0};
    }
  }
  return is_ident;
}

template <typename T>
std::string Mat<T>::ToString() const {
  // this method converts a matrix to something like
  //  [ a0 a1 a2 ... ]
  //  [ b0 b1 ..     ]
  // Each column is aligned according to the largest element in that column.

  const auto n = Rows();
  const auto m = Cols();

  if (!(n && m)) return "[ EMPTY_MATRIX ]";

  // convert all elements to strings and find the widest string in each column
  // since that will be used to properly align the final output.
  std::vector<std::string> elements;
  elements.reserve(n * m);
  std::vector<std::size_t> fills;
  fills.reserve(m);
  for (std::size_t j = 0; j < m; j++) {
    auto first = operator()(0, j).ToString();
    auto fill = first.size();
    elements.emplace_back(first);
    for (std::size_t i = 1; i < n; i++) {
      auto next = operator()(i, j).ToString();
      auto next_fill = next.size();
      if (next_fill > fill) fill = next_fill;
      elements.push_back(next);
    }
    fills.push_back(fill + 1);
  }

  std::stringstream ss;
  ss << "\n";
  for (std::size_t i = 0; i < n; i++) {
    ss << "[";
    for (std::size_t j = 0; j < m; j++) {
      ss << std::setfill(' ') << std::setw(fills[j]) << elements[j * n + i]
         << " ";
    }
    ss << "]";
    if (i < n - 1) ss << "\n";
  }
  return ss.str();
}

}  // namespace scl

#endif  // _SCL_MATH_MAT_H
