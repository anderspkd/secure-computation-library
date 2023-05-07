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

#ifndef SCL_MATH_OPS_H
#define SCL_MATH_OPS_H

#include <ostream>

namespace scl::math {

/**
 * @brief Provides binary <code>+</code> and <code>-</code>, and unary
 * <code>-</code> operators.
 *
 * Requires that the type \p T implements the <code>+=</code> and
 * <code>-=</code> operators, and a <code>Negate</code> function.
 */
template <typename T>
struct Add {
  /**
   * @brief Add two elements and return their sum.
   */
  friend T operator+(const T& lhs, const T& rhs) {
    T temp(lhs);
    return temp += rhs;
  }

  /**
   * @brief Subtract two elements and return their difference.
   */
  friend T operator-(const T& lhs, const T& rhs) {
    T temp(lhs);
    return temp -= rhs;
  }

  /**
   * @brief Return the negation of an element.
   */
  friend T operator-(const T& elem) {
    T temp(elem);
    return temp.Negate();
  }
};

/**
 * @brief Provides <code>*</code> and <code>/</code> operators.
 *
 * Requires that \p T implements the <code>*=</code> and <code>/=</code>
 * operators.
 */
template <typename T>
struct Mul {
  /**
   * @brief Multiply two elements and return their product.
   */
  friend T operator*(const T& lhs, const T& rhs) {
    T temp(lhs);
    return temp *= rhs;
  }

  /**
   * @brief Divide two elements and return their quotient.
   */
  friend T operator/(const T& lhs, const T& rhs) {
    T temp(lhs);
    return temp /= rhs;
  }
};

/**
 * @brief Provides <code>==</code> and <code>!=</code> operators.
 *
 * Requires that \p implements an <code>Equal(T)</code> function.
 */
template <typename T>
struct Eq {
  /**
   * @brief Compare two elements for equality.
   */
  friend bool operator==(const T& lhs, const T& rhs) {
    return lhs.Equal(rhs);
  }

  /**
   * @brief Compare two elements for inequality.
   */
  friend bool operator!=(const T& lhs, const T& rhs) {
    return !(lhs == rhs);
  }
};

/**
 * @brief Provides <code><<</code> syntax for printing to a string.
 *
 * Requires that \p implements a <code>ToString()</code> function.
 */
template <typename T>
struct Print {
  /**
   * @brief Write a string representation of an element to a stream.
   */
  friend std::ostream& operator<<(std::ostream& os, const T& r) {
    return os << r.ToString();
  }
};

}  // namespace scl::math

#endif  // SCL_MATH_OPS_H
