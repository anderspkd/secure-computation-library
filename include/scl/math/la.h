/**
 * @file la.h
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

#ifndef SCL_MATH_LA_H
#define SCL_MATH_LA_H

#include "scl/math/mat.h"
#include "scl/math/vec.h"

namespace scl {
namespace details {

/**
 * @brief Swap two rows of a matrix in-place.
 * @param A the matrix
 * @param k the first row
 * @param h the second row
 */
template <typename T>
void SwapRows(Mat<T>& A, std::size_t k, std::size_t h) {
  if (k != h) {
    T temp;
    for (std::size_t i = 0; i < A.Cols(); ++i) {
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
template <typename T>
void MultiplyRow(Mat<T>& A, std::size_t row, const T& m) {
  for (std::size_t j = 0; j < A.Cols(); ++j) A(row, j) *= m;
}

/**
 * @brief Add a mutliple of one row to another in a matrix.
 * @param A the matrix
 * @param dst the row that is mutated
 * @param op the row that is added to \p dst
 * @param m the multiple
 */
template <typename T>
void AddRows(Mat<T>& A, std::size_t dst, std::size_t op, const T& m) {
  for (std::size_t j = 0; j < A.Cols(); ++j) A(dst, j) += A(op, j) * m;
}

/**
 * @brief Bring a matrix into reduced row echelon form in-place.
 * @param A the matrix to bring into RREF
 */
template <typename T>
void RowReduceInPlace(Mat<T>& A) {
  std::size_t n = A.Rows();
  std::size_t m = A.Cols();
  std::size_t r = 0;
  std::size_t c = 0;
  const T zero;

  while (r < n && c < m) {
    // find pivot in current column
    auto pivot = r;
    while (pivot < n && A(pivot, c) == zero) pivot++;

    if (pivot == n) {
      // this column was all 0, so go to next one
      c++;
    } else {
      SwapRows(A, pivot, r);

      // make leading coefficient of this row 1.
      auto pv = A(r, c).Inverse();
      MultiplyRow(A, r, pv);

      // finally, for each row that is not r, subtract a multiple of row r.
      for (std::size_t k = 0; k < n; ++k) {
        if (k == r) continue;
        // skip row if leading coefficient of that row is 0.
        auto t = A(k, c);
        if (t != zero) AddRows(A, k, r, -t);
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
template <typename T>
int GetPivotInColumn(const Mat<T>& A, int col) {
  T zero;
  int i = A.Rows();
  while (i-- > 0) {
    if (A(i, col) != zero) {
      for (int k = 0; k < col - 1; ++k) {
        if (A(i, k) != zero) return -1;
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
template <typename T>
std::size_t FindFirstNonZeroRow(const Mat<T>& A) {
  std::size_t nzr = A.Rows();
  const auto m = A.Cols();
  while (nzr-- > 0) {
    bool non_zero = false;
    for (std::size_t j = 0; j < m; ++j) {
      if (A(nzr, j) != T{}) {
        non_zero = true;
        break;
      }
    }
    if (non_zero) break;
  }
  return nzr;
}

/**
 * @brief Extract a solution from a matrix in RREF.
 *
 * Given a matrix \p A in RREF that represents a system of equations, this
 * function extracts a solution for said system. The system is assumed to be
 * consistent, but may contain free variables. In those cases, the free
 * variables are given the value 1 (in the field).
 *
 * @param A the matrix
 * @return the solution.
 */
template <typename T>
Vec<T> ExtractSolution(const Mat<T>& A) {
  const auto n = A.Rows();
  const auto m = A.Cols();

  Vec<T> x(m - 1);
  auto i = FindFirstNonZeroRow(A);
  // we remove (n - i) rows, which means setting the corresponding variables to
  // 0.
  int c = m - 2 - (n - i - 1);
  for (; c >= 0; c--) {
    const auto p = GetPivotInColumn(A, c);
    if (p == -1) {
      // a free variable just gets set to 1.
      x[c] = T{1};
    } else {
      T sum;
      for (std::size_t j = p + 1; j < n; ++j) sum += A(i, j) * x[j];
      x[c] = A(i, m - 1) - sum;
      i--;
    }
  }
  return x;
}  // LCOV_EXCL_LINE

/**
 * @brief Check if a linear system has a solution.
 *
 * Determines if a linear system given by an augmented matrix in RREF has a
 * solution, aka. is consistent. if called with \p unique_only set to
 * <code>true</code>, then only systems with a unique solution are considered.
 *
 * @param A an augmented matrix in RREF
 * @param unique_only indicates if only unique solutions should be considered
 * @return true if \f$Ax = b\f$ has a solution and false otherwise.
 */
template <typename T>
bool HasSolution(const Mat<T>& A, bool unique_only) {
  auto n = A.Rows();
  auto m = A.Cols();
  T zero;
  for (std::size_t i = 0; i < n; ++i) {
    bool all_zero = true;
    for (std::size_t j = 0; j < m - 1; ++j) {
      all_zero &= A(i, j) == zero;
    }
    // No solution is the case when Rank(A) != Rank(A') where A' is A without
    // the last column (the augmentation). I.e., when row(A', i) == 0, but
    // row(A, i) != 0.
    if (unique_only) {
      if (all_zero) return false;
    } else {
      if (all_zero && A(i, m - 1) != zero) return false;
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
template <typename T>
Mat<T> CreateAugmentedMatrix(const Mat<T>& A, const Mat<T>& B) {
  auto n = A.Rows();
  auto m = A.Cols();
  auto k = B.Cols();
  Mat<T> aug(n, m + k);
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
template <typename T>
Mat<T> CreateAugmentedMatrix(const Mat<T>& A, const Vec<T>& b) {
  return CreateAugmentedMatrix(A, b.ToColumnMatrix());
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
template <typename T>
bool SolveLinearSystem(Vec<T>& x, const Mat<T>& A, const Vec<T>& b) {
  if (A.Rows() != b.Size()) {
    throw std::invalid_argument("malformed system of equations");
  }

  auto aug = CreateAugmentedMatrix(A, b);

  RowReduceInPlace(aug);
  if (!HasSolution(aug, true)) {
    return false;
  }

  x = ExtractSolution(aug);
  return true;
}

}  // namespace details
}  // namespace scl

#endif  // SCL_MATH_LA_H
