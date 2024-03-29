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

#ifndef SCL_MATH_MAT_H
#define SCL_MATH_MAT_H

#include <cstdint>
#include <cstring>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "scl/math/ff.h"
#include "scl/math/lagrange.h"
#include "scl/math/ops.h"
#include "scl/math/vec.h"
#include "scl/util/prg.h"
#include "scl/util/traits.h"

namespace scl::math {

template <typename Elem>
class Vec;

/**
 * @brief Matrix.
 */
template <typename Elem>
class Mat : Print<Mat<Elem>> {
 public:
  /**
   * @brief The type of the matrix elements.
   */
  using ValueType = Elem;

  /**
   * @brief The type of a dimension (row or column count).
   */
  using SizeType = std::uint32_t;

  /**
   * @brief Read a matrix from a stream of bytes.
   * @param n the number of rows
   * @param m the number of columns
   * @param src the bytes
   * @return a matrix.
   */
  static Mat<Elem> Read(std::size_t n, std::size_t m, const unsigned char* src);

  /**
   * @brief Create a Matrix and populate it with random elements.
   * @param n the number of rows
   * @param m the number of columns
   * @param prg the prg used to generate random elements
   * @return a Matrix with random elements.
   */
  static Mat<Elem> Random(std::size_t n, std::size_t m, util::PRG& prg);

  /**
   * @brief Create an N-by-M Vandermonde matrix.
   * @param n the number of rows.
   * @param m the number of columns.
   * @param xs vector containing the x values to use.
   * @return a Vandermonde matrix.
   *
   * Let \p xs be a list \f$(x_1, x_2, \dots, x_n)\f$ where \f$x_i \neq x_j\f$
   * for all \f$i\neq j\f$. A <i>Vandermonde</i> matrix, is the \f$n\times m\f$
   * matrix
   *
   * \f$
   * V =
   * \begin{bmatrix}
   * 1 & x_1 & x_1^2 & \dots & x_1^{m-1} \\
   * 1 & x_2 & x_2^2 & \dots & x_2^{m-1} \\
   * \ldots \\
   * 1 & x_n & x_n^2 & \dots & x_n^{m-1}
   * \end{bmatrix}
   * \f$
   *
   * @see https://en.wikipedia.org/wiki/Vandermonde_matrix
   */
  static Mat<Elem> Vandermonde(std::size_t n,
                               std::size_t m,
                               const Vec<Elem>& xs);

  /**
   * @brief Create an N-by-M Vandermonde matrix.
   * @param n the number of rows.
   * @param m the number of columns.
   * @return a Vandermonde matrix.
   *
   * This function returns a Vandermonde matrix using \f$(1, 2, \dots, n + 1)\f$
   * as the set of x values.
   *
   * @see Mat::Vandermonde.
   */
  static Mat<Elem> Vandermonde(std::size_t n, std::size_t m) {
    return Mat<Elem>::Vandermonde(n, m, Vec<Elem>::Range(1, n + 1));
  }

  /**
   * @brief Create an N-by-M hyper-invertible matrix.
   * @param n the number of rows.
   * @param m the number of columns.
   * @return a Hyperinvertible matrix.
   *
   * A hyper-invertible matrix is a matrix where every square sub-matrix is
   * invertible.
   */
  static Mat<Elem> HyperInvertible(std::size_t n, std::size_t m);

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
  static Mat<Elem> FromVector(std::size_t n,
                              std::size_t m,
                              const std::vector<Elem>& vec) {
    if (vec.size() != n * m) {
      throw std::invalid_argument("invalid dimensions");
    }
    return Mat<Elem>(n, m, vec);
  }

  /**
   * @brief Construct an n-by-n identity matrix.
   */
  static Mat<Elem> Identity(std::size_t n) {
    Mat<Elem> I(n);
    for (std::size_t i = 0; i < n; ++i) {
      I(i, i) = Elem(1);
    }
    return I;
  }  // LCOV_EXCL_LINE

  /**
   * @brief Construct an empty 0-by-0 matrix.
   */
  Mat() : m_rows(0), m_cols(0) {}

  /**
   * @brief Create an N-by-M matrix with default initialized values.
   * @param n the number of rows
   * @param m the number of columns
   */
  explicit Mat(std::size_t n, std::size_t m) {
    if (n == 0 || m == 0) {
      throw std::invalid_argument("n or m cannot be 0");
    }
    std::vector<Elem> v(n * m);
    m_rows = n;
    m_cols = m;
    m_values = v;
  }

  /**
   * @brief Create a square matrix with default initialized values.
   * @param n the dimensions of the matrix
   */
  explicit Mat(std::size_t n) : Mat(n, n){};

  /**
   * @brief The number of rows of this matrix.
   */
  SizeType Rows() const {
    return m_rows;
  }

  /**
   * @brief The number of columns of this matrix.
   */
  SizeType Cols() const {
    return m_cols;
  }

  /**
   * @brief Provides mutable access to a matrix element.
   * @param row the row of the element being queried
   * @param column the column of the element being queried
   * @return an element of the matrix.
   */
  Elem& operator()(std::size_t row, std::size_t column) {
    return m_values[m_cols * row + column];
  }

  /**
   * @brief Provides read-only access to a matrix element.
   * @param row the row of the element being queried
   * @param column the column of the element being queried
   * @return an element of the matrix.
   */
  Elem operator()(std::size_t row, std::size_t column) const {
    return m_values[m_cols * row + column];
  }

  /**
   * @brief Add this matrix with another matrix of the same dimensions.
   * @param other the other matrix
   * @return The entry-wise sum of this matrix and \p other.
   * @throws std::illegal_argument if the dimensions of this and \p other are
   *         not equal.
   */
  Mat Add(const Mat& other) const {
    Mat copy(m_rows, m_cols, m_values);
    return copy.AddInPlace(other);
  }

  /**
   * @brief Add this matrix with another matrix of the same dimensions in-place.
   * @param other the other matrix
   * @return The entry-wise sum of this matrix and \p other.
   * @throws std::illegal_argument if the dimensions of this and \p other are
   *         not equal.
   */
  Mat& AddInPlace(const Mat& other) {
    EnsureCompatible(other);
    auto n = m_values.size();
    for (std::size_t i = 0; i < n; ++i) {
      m_values[i] += other.m_values[i];
    }
    return *this;
  }

  /**
   * @brief Subtract this matrix with another matrix of the same dimensions.
   * @param other the other matrix
   * @return the difference of this and \p other
   * @throws std::illegal_argument if the dimensions of this and \p do not
   *         match.
   */
  Mat Subtract(const Mat& other) const {
    Mat copy(m_rows, m_cols, m_values);
    return copy.SubtractInPlace(other);
  }

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
    auto n = m_values.size();
    for (std::size_t i = 0; i < n; i++) {
      m_values[i] -= other.m_values[i];
    }
    return *this;
  }

  /**
   * @brief Multiply this matrix with another matrix of the same dimensions.
   * @param other the other matrix
   * @return the entry-wise product of this and \p other
   * @throws std::illegal_argument if the dimensions of this and \p do not
   *         match.
   */
  Mat MultiplyEntryWise(const Mat& other) const {
    Mat copy(m_rows, m_cols, m_values);
    return copy.MultiplyEntryWiseInPlace(other);
  }

  /**
   * @brief Multiply this matrix with another matrix of the same dimensions.
   * @param other the other matrix
   * @return the product of this and \p other
   * @throws std::illegal_argument if the dimensions of this and \p do not
   *         match.
   */
  Mat& MultiplyEntryWiseInPlace(const Mat& other) {
    EnsureCompatible(other);
    auto n = m_values.size();
    for (std::size_t i = 0; i < n; i++) {
      m_values[i] *= other.m_values[i];
    }
    return *this;
  }

  /**
   * @brief Performs a matrix multiplication.
   * @param other the other matrix
   * @return the matrix product of this and \p other.
   * @throws std::illegal_argument if the dimensions of the inputs are
   *         incompatible.
   */
  Mat Multiply(const Mat& other) const;

  /**
   * @brief Performs a matrix vector product.
   * @param vector the vector.
   * @return the result vector.
   *
   * This computes \f$A\cdot x\f$ where \f$A\f$ is a \f$n\times m\f$ matrix and
   * \f$x\f$ a length \f$m\f$ vector. The return value is a vector \f$y\f$ of
   * length \f$n\f$.
   */
  Vec<Elem> Multiply(const Vec<Elem>& vector) const;

  /**
   * @brief Multiply this matrix with a scalar
   * @param scalar the scalar
   * @return this scaled by \p scalar.
   */
  template <
      typename Scalar,
      std::enable_if_t<util::HasOperatorMul<Elem, Scalar>::value, bool> = true>
  Mat ScalarMultiply(const Scalar& scalar) const {
    Mat copy(m_rows, m_cols, m_values);
    return copy.ScalarMultiplyInPlace(scalar);
  }

  /**
   * @brief Multiply this matrix with a scalar in-place.
   * @param scalar the scalar
   * @return this scaled by \p scalar.
   */
  template <
      typename Scalar,
      std::enable_if_t<util::HasOperatorMul<Elem, Scalar>::value, bool> = true>
  Mat& ScalarMultiplyInPlace(const Scalar& scalar) {
    for (auto& v : m_values) {
      v *= scalar;
    }
    return *this;
  }

  /**
   * @brief Check if this matrix is square.
   */
  bool IsSquare() const {
    return Rows() == Cols();
  }

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
    if (rows * cols != Rows() * Cols()) {
      throw std::invalid_argument("cannot resize matrix");
    }
    m_rows = rows;
    m_cols = cols;
    return *this;
  }

  /**
   * @brief Returns true if this matrix is the identity matrix.
   */
  bool IsIdentity() const;

  /**
   * @brief Return a string representation of this matrix.
   */
  std::string ToString() const;

  /**
   * @brief Test if this matrix is equal to another.
   */
  bool Equals(const Mat& other) const {
    if (Rows() != other.Rows() || Cols() != other.Cols()) {
      return false;
    }

    bool equal = true;
    for (std::size_t i = 0; i < m_values.size(); i++) {
      equal &= m_values[i] == other.m_values[i];
    }
    return equal;
  }

  /**
   * @brief Write this matrix to a buffer.
   * @param dest where to write the matrix.
   *
   * This function just writes the content of the matrix.
   */
  void Write(unsigned char* dest) const;

  /**
   * @brief The size of a matrix when serialized in bytes.
   */
  std::size_t ByteSize() const {
    return Cols() * Rows() * Elem::ByteSize();
  }

 private:
  Mat(std::size_t r, std::size_t c, std::vector<Elem> v)
      : m_rows(r), m_cols(c), m_values(v){};

  void EnsureCompatible(const Mat& other) {
    if (m_rows != other.m_rows || m_cols != other.m_cols) {
      throw std::invalid_argument("incompatible matrices");
    }
  }

  SizeType m_rows;
  SizeType m_cols;
  std::vector<Elem> m_values;

  friend class Vec<Elem>;
};

template <typename Elem>
Mat<Elem> Mat<Elem>::Read(std::size_t n,
                          std::size_t m,
                          const unsigned char* src) {
  const auto* ptr = src;
  auto total = n * m;

  // write all elements now that we know we'll not exceed the maximum read size.
  std::vector<Elem> elements;
  elements.reserve(total);
  for (std::size_t i = 0; i < total; i++) {
    elements.emplace_back(Elem::Read(ptr));
    ptr += Elem::ByteSize();
  }
  return Mat(n, m, elements);
}

template <typename Elem>
void Mat<Elem>::Write(unsigned char* dest) const {
  for (const auto& v : m_values) {
    v.Write(dest);
    dest += Elem::ByteSize();
  }
}

template <typename Elem>
Mat<Elem> Mat<Elem>::Random(std::size_t n, std::size_t m, util::PRG& prg) {
  std::size_t nelements = n * m;
  return Mat(n, m, Vec<Elem>::Random(nelements, prg).ToStlVector());
}

template <typename Elem>
Mat<Elem> Mat<Elem>::Vandermonde(std::size_t n,
                                 std::size_t m,
                                 const Vec<Elem>& xs) {
  if (xs.Size() != n) {
    throw std::invalid_argument("|xs| != number of rows");
  }

  Mat<Elem> v(n, m);
  for (std::size_t i = 0; i < n; ++i) {
    v(i, 0) = Elem(1);
    for (std::size_t j = 1; j < m; ++j) {
      v(i, j) = v(i, j - 1) * xs[i];
    }
  }
  return v;
}  // LCOV_EXCL_LINE

template <typename Elem>
Mat<Elem> Mat<Elem>::HyperInvertible(std::size_t n, std::size_t m) {
  Mat<Elem> him(n, m);

  const auto vs = Vec<Elem>::Range(1, m + 1);

  for (std::size_t i = 0; i < n; ++i) {
    const auto r = ComputeLagrangeBasis(vs, -i);
    for (std::size_t j = 0; j < m; ++j) {
      him(i, j) = r[j];
    }
  }
  return him;
}

template <typename Elem>
Mat<Elem> Mat<Elem>::Multiply(const Mat<Elem>& other) const {
  if (Cols() != other.Rows()) {
    throw std::invalid_argument("matmul: this->Cols() != that->Rows()");
  }
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
}  // LCOV_EXCL_LINE

template <typename Elem>
Vec<Elem> Mat<Elem>::Multiply(const Vec<Elem>& vector) const {
  if (Cols() != vector.Size()) {
    throw std::invalid_argument("matmul: this->Cols() != vec.Size()");
  }

  std::vector<Elem> result;
  result.reserve(Rows());

  for (std::size_t i = 0; i < Rows(); ++i) {
    auto b = m_values.begin() + i * Cols();
    auto e = m_values.begin() + (i + 1) * Cols();
    result.emplace_back(UncheckedInnerProd<Elem>(b, e, vector.begin()));
  }

  return result;
}

template <typename Elem>
Mat<Elem> Mat<Elem>::Transpose() const {
  Mat t(Cols(), Rows());
  for (std::size_t i = 0; i < Rows(); i++) {
    for (std::size_t j = 0; j < Cols(); j++) {
      t(j, i) = operator()(i, j);
    }
  }
  return t;
}

template <typename Elem>
bool Mat<Elem>::IsIdentity() const {
  if (!IsSquare()) {
    return false;
  }

  bool is_ident = true;
  for (std::size_t i = 0; i < Rows(); ++i) {
    for (std::size_t j = 0; j < Cols(); ++j) {
      if (i == j) {
        is_ident &= operator()(i, j) == Elem{1};
      } else {
        is_ident &= operator()(i, j) == Elem{0};
      }
    }
  }
  return is_ident;
}

template <typename Elem>
std::string Mat<Elem>::ToString() const {
  // this method converts a matrix to something like
  //  [ a0 a1 a2 ... ]
  //  [ b0 b1 ..     ]
  // Each column is aligned according to the largest element in that column.

  const auto n = Rows();
  const auto m = Cols();

  if (!(n && m)) {
    return "[ EMPTY MATRIX ]";
  }

  // convert all elements to strings and find the widest string in each column
  // since that will be used to properly align the final output.
  std::vector<std::string> elements;
  elements.reserve(n * m);
  std::vector<int> fills;
  fills.reserve(m);
  for (std::size_t j = 0; j < m; j++) {
    auto first = operator()(0, j).ToString();
    auto fill = first.size();
    elements.emplace_back(first);
    for (std::size_t i = 1; i < n; i++) {
      auto next = operator()(i, j).ToString();
      auto next_fill = next.size();
      if (next_fill > fill) {
        fill = next_fill;
      }
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
    if (i < n - 1) {
      ss << "\n";
    }
  }
  return ss.str();
}

}  // namespace scl::math

#endif  // SCL_MATH_MAT_H
