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

#ifndef SCL_MATH_MATRIX_H
#define SCL_MATH_MATRIX_H

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
#include "scl/math/vector.h"
#include "scl/serialization/serializer.h"
#include "scl/util/prg.h"

namespace scl {
namespace math {

template <typename Elem>
class Vector;

/**
 * @brief Matrix.
 */
template <typename ELEMENT>
class Matrix final {
 public:
  friend struct seri::Serializer<Matrix<ELEMENT>>;

  /**
   * @brief The type of the matrix elements.
   */
  using ValueType = ELEMENT;

  /**
   * @brief Create a Matrix and populate it with random elements.
   * @param n the number of rows
   * @param m the number of columns
   * @param prg the prg used to generate random elements
   * @return a Matrix with random elements.
   */
  static Matrix<ELEMENT> random(std::size_t n, std::size_t m, util::PRG& prg);

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
  static Matrix<ELEMENT> vandermonde(std::size_t n,
                                     std::size_t m,
                                     const Vector<ELEMENT>& xs);

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
  static Matrix<ELEMENT> vandermonde(std::size_t n, std::size_t m) {
    return Matrix<ELEMENT>::vandermonde(n, m, Vector<ELEMENT>::range(1, n + 1));
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
  static Matrix<ELEMENT> hyperInvertible(std::size_t n, std::size_t m);

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
  static Matrix<ELEMENT> fromVector(std::size_t n,
                                    std::size_t m,
                                    const std::vector<ELEMENT>& vec) {
    if (vec.size() != n * m) {
      throw std::invalid_argument("invalid dimensions");
    }
    return Matrix<ELEMENT>(n, m, vec);
  }

  /**
   * @brief Construct an n-by-n identity matrix.
   */
  static Matrix<ELEMENT> identity(std::size_t n) {
    Matrix<ELEMENT> I(n);
    for (std::size_t i = 0; i < n; ++i) {
      I(i, i) = ELEMENT(1);
    }
    return I;
  }  // LCOV_EXCL_LINE

  /**
   * @brief Construct an empty 0-by-0 matrix.
   */
  Matrix() : m_rows(0), m_cols(0) {}

  /**
   * @brief Create an N-by-M matrix with default initialized values.
   * @param n the number of rows
   * @param m the number of columns
   */
  explicit Matrix(std::size_t n, std::size_t m) {
    if (n == 0 || m == 0) {
      throw std::invalid_argument("n or m cannot be 0");
    }
    std::vector<ELEMENT> v(n * m);
    m_rows = n;
    m_cols = m;
    m_values = v;
  }

  /**
   * @brief Create a square matrix with default initialized values.
   * @param n the dimensions of the matrix
   */
  explicit Matrix(std::size_t n) : Matrix(n, n){};

  /**
   * @brief The number of rows of this matrix.
   */
  std::size_t rows() const {
    return m_rows;
  }

  /**
   * @brief The number of columns of this matrix.
   */
  std::size_t cols() const {
    return m_cols;
  }

  /**
   * @brief Provides mutable access to a matrix element.
   * @param row the row of the element being queried
   * @param column the column of the element being queried
   * @return an element of the matrix.
   */
  ELEMENT& operator()(std::size_t row, std::size_t column) {
    return m_values[m_cols * row + column];
  }

  /**
   * @brief Provides read-only access to a matrix element.
   * @param row the row of the element being queried
   * @param column the column of the element being queried
   * @return an element of the matrix.
   */
  ELEMENT operator()(std::size_t row, std::size_t column) const {
    return m_values[m_cols * row + column];
  }

  /**
   * @brief Add this matrix with another matrix of the same dimensions.
   * @param other the other matrix
   * @return The entry-wise sum of this matrix and \p other.
   * @throws std::illegal_argument if the dimensions of this and \p other are
   *         not equal.
   */
  Matrix add(const Matrix& other) const {
    Matrix copy(m_rows, m_cols, m_values);
    return copy.addInPlace(other);
  }

  /**
   * @brief Add this matrix with another matrix of the same dimensions in-place.
   * @param other the other matrix
   * @return The entry-wise sum of this matrix and \p other.
   * @throws std::illegal_argument if the dimensions of this and \p other are
   *         not equal.
   */
  Matrix& addInPlace(const Matrix& other) {
    ensureCompatible(other);
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
  Matrix subtract(const Matrix& other) const {
    Matrix copy(m_rows, m_cols, m_values);
    return copy.subtractInPlace(other);
  }

  /**
   * @brief Subtract this matrix with another matrix of the same dimensions
   *        in-place.
   * @param other the other matrix
   * @return the difference of this and \p other
   * @throws std::illegal_argument if the dimensions of this and \p do not
   *         match.
   */
  Matrix& subtractInPlace(const Matrix& other) {
    ensureCompatible(other);
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
  Matrix multiplyEntryWise(const Matrix& other) const {
    Matrix copy(m_rows, m_cols, m_values);
    return copy.multiplyEntryWiseInPlace(other);
  }

  /**
   * @brief Multiply this matrix with another matrix of the same dimensions.
   * @param other the other matrix
   * @return the product of this and \p other
   * @throws std::illegal_argument if the dimensions of this and \p do not
   *         match.
   */
  Matrix& multiplyEntryWiseInPlace(const Matrix& other) {
    ensureCompatible(other);
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
  Matrix multiply(const Matrix& other) const;

  /**
   * @brief Performs a matrix vector product.
   * @param vector the vector.
   * @return the result vector.
   *
   * This computes \f$A\cdot x\f$ where \f$A\f$ is a \f$n\times m\f$ matrix and
   * \f$x\f$ a length \f$m\f$ vector. The return value is a vector \f$y\f$ of
   * length \f$n\f$.
   */
  Vector<ELEMENT> multiply(const Vector<ELEMENT>& vector) const;

  /**
   * @brief Multiply this matrix with a scalar
   * @param scalar the scalar
   * @return this scaled by \p scalar.
   */
  template <typename SCALAR>
    requires requires(ELEMENT a, ELEMENT b) { (a) * (b); }
  Matrix scalarMultiply(const SCALAR& scalar) const {
    Matrix copy(m_rows, m_cols, m_values);
    return copy.scalarMultiplyInPlace(scalar);
  }

  /**
   * @brief Multiply this matrix with a scalar in-place.
   * @param scalar the scalar
   * @return this scaled by \p scalar.
   */
  template <typename SCALAR>
    requires requires(ELEMENT a, ELEMENT b) { (a) * (b); }
  Matrix& scalarMultiplyInPlace(const SCALAR& scalar) {
    for (auto& v : m_values) {
      v *= scalar;
    }
    return *this;
  }

  /**
   * @brief Check if this matrix is square.
   */
  bool isSquare() const {
    return rows() == cols();
  }

  /**
   * @brief Transpose this matrix.
   * @return the transpose of this.
   */
  Matrix transpose() const;

  /**
   * @brief Resize this matrix.
   * @param new_rows the new row count
   * @param new_cols the new column count
   */
  Matrix& resize(std::size_t new_rows, std::size_t new_cols) {
    if (new_rows * new_cols != rows() * cols()) {
      throw std::invalid_argument("cannot resize matrix");
    }
    m_rows = new_rows;
    m_cols = new_cols;
    return *this;
  }

  /**
   * @brief Returns true if this matrix is the identity matrix.
   */
  bool isIdentity() const;

  /**
   * @brief Compute the inverse of this matrix, if possible.
   * @throws std::invalid_argument if the matrix is not square.
   *
   * This function computes the inverse of a matrix using Guassian
   * elimination. No checks are done as to whether such an inverse exists.
   */
  Matrix invert() const;

  /**
   * @brief Return a string representation of this matrix.
   */
  std::string toString() const;

  /**
   * @brief Write a string representation of this matrix to a stream.
   */
  friend std::ostream& operator<<(std::ostream& os, const Matrix& matrix) {
    return os << matrix.toString();
  }

  /**
   * @brief Test if this matrix is equal to another.
   */
  bool equals(const Matrix& other) const {
    if (rows() != other.rows() || cols() != other.cols()) {
      return false;
    }

    bool equal = true;
    for (std::size_t i = 0; i < m_values.size(); i++) {
      equal &= m_values[i] == other.m_values[i];
    }
    return equal;
  }

  /**
   * @brief The size of a matrix when serialized in bytes.
   */
  std::size_t byteSize() const {
    return cols() * rows() * ELEMENT::byteSize();
  }

 private:
  Matrix(std::size_t r, std::size_t c, std::vector<ELEMENT> v)
      : m_rows(r), m_cols(c), m_values(v){};

  void ensureCompatible(const Matrix& other) {
    if (m_rows != other.m_rows || m_cols != other.m_cols) {
      throw std::invalid_argument("incompatible matrices");
    }
  }

  std::size_t m_rows;
  std::size_t m_cols;
  std::vector<ELEMENT> m_values;

  friend class Vector<ELEMENT>;
};

template <typename ELEMENT>
Matrix<ELEMENT> Matrix<ELEMENT>::random(std::size_t n,
                                        std::size_t m,
                                        util::PRG& prg) {
  std::size_t nelements = n * m;
  return Matrix(n, m, Vector<ELEMENT>::random(nelements, prg).toStlVector());
}

template <typename ELEMENT>
Matrix<ELEMENT> Matrix<ELEMENT>::vandermonde(std::size_t n,
                                             std::size_t m,
                                             const Vector<ELEMENT>& xs) {
  if (xs.size() != n) {
    throw std::invalid_argument("|xs| != number of rows");
  }

  Matrix<ELEMENT> v(n, m);
  for (std::size_t i = 0; i < n; ++i) {
    v(i, 0) = ELEMENT(1);
    for (std::size_t j = 1; j < m; ++j) {
      v(i, j) = v(i, j - 1) * xs[i];
    }
  }
  return v;
}  // LCOV_EXCL_LINE

template <typename ELEMENT>
Matrix<ELEMENT> Matrix<ELEMENT>::hyperInvertible(std::size_t n, std::size_t m) {
  Matrix<ELEMENT> him(n, m);

  const auto vs = Vector<ELEMENT>::range(1, m + 1);

  for (std::size_t i = 0; i < n; ++i) {
    const auto r = computeLagrangeBasis(vs, -i);
    for (std::size_t j = 0; j < m; ++j) {
      him(i, j) = r[j];
    }
  }
  return him;
}

template <typename ELEMENT>
Matrix<ELEMENT> Matrix<ELEMENT>::multiply(const Matrix<ELEMENT>& other) const {
  if (cols() != other.rows()) {
    throw std::invalid_argument("matmul: this->cols() != that->rows()");
  }
  const auto n = rows();
  const auto p = cols();
  const auto m = other.cols();

  Matrix result(n, m);
  for (std::size_t i = 0; i < n; i++) {
    for (std::size_t k = 0; k < p; k++) {
      for (std::size_t j = 0; j < m; j++) {
        result(i, j) += operator()(i, k) * other(k, j);
      }
    }
  }
  return result;
}  // LCOV_EXCL_LINE

template <typename ELEMENT>
Vector<ELEMENT> Matrix<ELEMENT>::multiply(const Vector<ELEMENT>& vector) const {
  if (cols() != vector.size()) {
    throw std::invalid_argument("matmul: this->cols() != vec.size()");
  }

  std::vector<ELEMENT> result;
  result.reserve(rows());

  for (std::size_t i = 0; i < rows(); ++i) {
    auto b = m_values.begin() + i * cols();
    auto e = m_values.begin() + (i + 1) * cols();
    result.emplace_back(innerProd<ELEMENT>(b, e, vector.begin()));
  }

  return result;
}

template <typename ELEMENT>
Matrix<ELEMENT> Matrix<ELEMENT>::transpose() const {
  Matrix t(cols(), rows());
  for (std::size_t i = 0; i < rows(); i++) {
    for (std::size_t j = 0; j < cols(); j++) {
      t(j, i) = operator()(i, j);
    }
  }
  return t;
}

template <typename ELEMENT>
bool Matrix<ELEMENT>::isIdentity() const {
  if (!isSquare()) {
    return false;
  }

  bool is_ident = true;
  for (std::size_t i = 0; i < rows(); ++i) {
    for (std::size_t j = 0; j < cols(); ++j) {
      if (i == j) {
        is_ident &= operator()(i, j) == ELEMENT{1};
      } else {
        is_ident &= operator()(i, j) == ELEMENT{0};
      }
    }
  }
  return is_ident;
}

/**
 * @brief Swap two rows of a matrix in-place.
 * @param A the matrix
 * @param k the first row
 * @param h the second row
 */
template <typename ELEMENT>
void swapRows(Matrix<ELEMENT>& A, std::size_t k, std::size_t h) {
  if (k != h) {
    ELEMENT temp;
    for (std::size_t i = 0; i < A.cols(); ++i) {
      temp = A(h, i);
      A(h, i) = A(k, i);
      A(k, i) = temp;
    }
  }
}

/**
 * @brief Multiply a row in a matrix by a constant.
 * @param A the matrix
 * @param row the row
 * @param m the constant
 */
template <typename ELEMENT>
void multiplyRow(Matrix<ELEMENT>& A, std::size_t row, const ELEMENT& m) {
  for (std::size_t j = 0; j < A.cols(); ++j) {
    A(row, j) *= m;
  }
}

/**
 * @brief Add a mutliple of one row to another in a matrix.
 * @param A the matrix
 * @param dst the row that is mutated
 * @param op the row that is added to \p dst
 * @param m the multiple
 */
template <typename ELEMENT>
void addRows(Matrix<ELEMENT>& A,
             std::size_t dst,
             std::size_t op,
             const ELEMENT& m) {
  for (std::size_t j = 0; j < A.cols(); ++j) {
    A(dst, j) += A(op, j) * m;
  }
}

/**
 * @brief Bring a matrix into reduced row echelon form in-place.
 * @param A the matrix to bring into RREF
 */
template <typename ELEMENT>
void rowReduceInPlace(Matrix<ELEMENT>& A) {
  std::size_t n = A.rows();
  std::size_t m = A.cols();
  std::size_t r = 0;
  std::size_t c = 0;
  const ELEMENT zero;

  while (r < n && c < m) {
    // find pivot in current column
    auto pivot = r;
    while (pivot < n && A(pivot, c) == zero) {
      pivot++;
    }

    if (pivot == n) {
      // this column was all 0, so go to next one
      c++;
    } else {
      swapRows(A, pivot, r);

      // make leading coefficient of this row 1.
      auto pv = A(r, c).inverse();
      multiplyRow(A, r, pv);

      // finally, for each row that is not r, subtract a multiple of row r.
      for (std::size_t k = 0; k < n; ++k) {
        if (k == r) {
          continue;
        }
        // skip row if leading coefficient of that row is 0.
        auto t = A(k, c);
        if (t != zero) {
          addRows(A, k, r, -t);
        }
      }
      r++;
      c++;
    }
  }
}

/**
 * @brief Finds the position of a pivot in a column, if any.
 * @param A a RREF matrix
 * @param col the column
 * @return The index of a pivot in \p col, or -1 if non exists
 * @note No validation is performed on any of the arguments.
 */
template <typename ELEMENT>
int getPivotInColumn(const Matrix<ELEMENT>& A, int col) {
  const auto zero = ELEMENT::zero();
  int i = A.rows();
  while (i-- > 0) {
    if (A(i, col) != zero) {
      for (int k = 0; k < col - 1; ++k) {
        if (A(i, k) != zero) {
          return -1;
        }
      }
      return i;
    }
  }
  return -1;
}

/**
 * @brief Finds the first non-zero row, starting from the "bottom" of a matrix
 *
 * Starting from the bottom, finds the first row that is non-zero in a matrix.
 * This function is used to determine rows that can be skipped when performing
 * back substitution.
 *
 * @param A the matrix
 * @return The first non-zero row
 */
template <typename ELEMENT>
std::size_t findFirstNonZeroRow(const Matrix<ELEMENT>& A) {
  const auto zero = ELEMENT::zero();
  std::size_t nzr = A.rows();
  const auto m = A.cols();
  while (nzr-- > 0) {
    bool non_zero = false;
    for (std::size_t j = 0; j < m; ++j) {
      if (A(nzr, j) != zero) {
        non_zero = true;
        break;
      }
    }
    if (non_zero) {
      break;
    }
  }
  return nzr;
}

/**
 * @brief Extract a solution from a matrix in RREF.
 * @param A the matrix
 * @return the solution.
 *
 * Given a matrix \p A in RREF that represents a system of equations, this
 * function extracts a solution for said system. The system is assumed to be
 * consistent, but may contain free variables. In those cases, the free
 * variables are given the value 1 (in the field).
 */
template <typename ELEMENT>
Vector<ELEMENT> extractSolution(const Matrix<ELEMENT>& A) {
  const auto n = A.rows();
  const auto m = A.cols();

  Vector<ELEMENT> x(m - 1);
  auto i = findFirstNonZeroRow(A);
  // we remove (n - i) rows, which means setting the corresponding variables to
  // 0.
  int c = m - 2 - (n - i - 1);
  for (; c >= 0; c--) {
    const auto p = getPivotInColumn(A, c);
    if (p == -1) {
      // a free variable just gets set to 1.
      x[c] = ELEMENT{1};
    } else {
      auto sum = ELEMENT::zero();
      for (std::size_t j = p + 1; j < n; ++j) {
        sum += A(i, j) * x[j];
      }
      x[c] = A(i, m - 1) - sum;
      i--;
    }
  }
  return x;
}  // LCOV_EXCL_LINE

/**
 * @brief Check if a linear system has a solution.
 * @param A an augmented matrix in RREF
 * @param unique_only indicates if only unique solutions should be considered
 * @return true if \f$Ax = b\f$ has a solution and false otherwise.
 *
 * Determines if a linear system given by an augmented matrix in RREF has a
 * solution, aka. is consistent. if called with \p unique_only set to
 * <code>true</code>, then only systems with a unique solution are considered.
 */
template <typename ELEMENT>
bool hasSolution(const Matrix<ELEMENT>& A, bool unique_only) {
  auto n = A.rows();
  auto m = A.cols();
  ELEMENT zero;
  for (std::size_t i = 0; i < n; ++i) {
    bool all_zero = true;
    for (std::size_t j = 0; j < m - 1; ++j) {
      all_zero &= A(i, j) == zero;
    }
    // No solution is the case when Rank(A) != Rank(A') where A' is A without
    // the last column (the augmentation). I.e., when row(A', i) == 0, but
    // row(A, i) != 0.
    if (unique_only) {
      if (all_zero) {
        return false;
      }
    } else {
      if (all_zero && A(i, m - 1) != zero) {
        return false;
      }
    }
  }
  return true;
}

/**
 * @brief Creates an augmented matrix from two matrices.
 * @param A the first matrix
 * @param B the second matrix
 * @return A matrix aug <code>[A | B]</code>.
 */
template <typename ELEMENT>
Matrix<ELEMENT> createAugmentedMatrix(const Matrix<ELEMENT>& A,
                                      const Matrix<ELEMENT>& B) {
  auto n = A.rows();
  auto m = A.cols();
  auto k = B.cols();
  Matrix<ELEMENT> aug(n, m + k);
  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t j = 0; j < m; ++j) {
      aug(i, j) = A(i, j);
    }
    for (std::size_t j = m; j < m + k; ++j) {
      aug(i, j) = B(i, j - m);
    }
  }
  return aug;
}

/**
 * @brief Create an augmented matrix from a matrix and a vector
 * @param A a matrix
 * @param b a vector
 * @return A matrix aug <code>[A | b]</code>.
 */
template <typename ELEMENT>
Matrix<ELEMENT> createAugmentedMatrix(const Matrix<ELEMENT>& A,
                                      const Vector<ELEMENT>& b) {
  return createAugmentedMatrix(A, b.toColumnMatrix());
}

/**
 * @brief Solves a linear system of equations \f$Ax = b\f$.
 * @param x where to store the solution
 * @param A the matrix of coefficients
 * @param b the equation values
 * @return true if a unique solution was found and false otherwise
 * @throws std::invalid_argument if the number of rows in \p A does not
 * match the size of \p b.
 */
template <typename ELEMENT>
bool solveLinearSystem(Vector<ELEMENT>& x,
                       const Matrix<ELEMENT>& A,
                       const Vector<ELEMENT>& b) {
  if (A.rows() != b.size()) {
    throw std::invalid_argument("malformed system of equations");
  }

  auto aug = createAugmentedMatrix(A, b);

  rowReduceInPlace(aug);
  if (!hasSolution(aug, true)) {
    return false;
  }

  x = extractSolution(aug);
  return true;
}

template <typename ELEMENT>
Matrix<ELEMENT> Matrix<ELEMENT>::invert() const {
  if (!isSquare()) {
    throw std::invalid_argument("cannot invert non-square matrix");
  }

  const std::size_t n = cols();
  const Matrix<ELEMENT> id = identity(n);

  auto aug = createAugmentedMatrix(*this, id);
  rowReduceInPlace(aug);

  Matrix<ELEMENT> inv(n, n);
  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t j = 0; j < n; ++j) {
      inv(i, j) = aug(i, n + j);
    }
  }

  return inv;
}

template <typename ELEMENT>
std::string Matrix<ELEMENT>::toString() const {
  // this method converts a matrix to something like
  //  [ a0 a1 a2 ... ]
  //  [ b0 b1 ..     ]
  // Each column is aligned according to the largest element in that column.

  const auto n = rows();
  const auto m = cols();

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
    auto first = operator()(0, j).toString();
    auto fill = first.size();
    elements.emplace_back(first);
    for (std::size_t i = 1; i < n; i++) {
      auto next = operator()(i, j).toString();
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

}  // namespace math

namespace seri {

/**
 * @brief Serializer specialization for a math::Mat.
 */
template <typename ELEMENT>
struct Serializer<math::Matrix<ELEMENT>> {
 private:
  // type used to denote a dimension
  using DimType = std::uint32_t;

  // serializer for vector
  using S_vec = Serializer<std::vector<ELEMENT>>;

  // serializer for the dimension
  using S_dim = Serializer<DimType>;

 public:
  /**
   * @brief Size of a matrix.
   * @param mat the matrix.
   */
  static std::size_t sizeOf(const math::Matrix<ELEMENT>& mat) {
    return S_vec::sizeOf(mat.m_values) + 2 * sizeof(DimType);
  }

  /**
   * @brief Write a matrix to a buffer.
   * @param mat the matrix.
   * @param buf the buffer.
   */
  static std::size_t write(const math::Matrix<ELEMENT>& mat,
                           unsigned char* buf) {
    std::size_t offset = 0;
    offset = S_dim::write(static_cast<DimType>(mat.rows()), buf);
    offset += S_dim::write(static_cast<DimType>(mat.cols()), buf + offset);
    S_vec::write(mat.m_values, buf + offset);
    return sizeOf(mat);
  }

  /**
   * @brief Read a matrix from a buffer.
   * @param mat where to store the matrix after reading.
   * @param buf the buffer.
   * @return the number of bytes read.
   */
  static std::size_t read(math::Matrix<ELEMENT>& mat,
                          const unsigned char* buf) {
    DimType rows;
    DimType cols;
    std::size_t offset = 0;
    offset = S_dim::read(rows, buf);
    offset += S_dim::read(cols, buf + offset);
    std::vector<ELEMENT> elements;
    S_vec::read(elements, buf + offset);
    mat = math::Matrix<ELEMENT>(rows, cols, std::move(elements));
    return sizeOf(mat);
  }
};

}  // namespace seri
}  // namespace scl

#endif  // SCL_MATH_MATRIX_H
