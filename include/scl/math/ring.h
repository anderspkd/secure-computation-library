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

#ifndef SCL_MATH_RING_H
#define SCL_MATH_RING_H

#include <ostream>
#include <type_traits>

namespace scl::math {

/**
 * @brief Derives some basic operations on Ring elements via. CRTP.
 */
template <typename T>
struct Ring {
  /**
   * @brief Add two elements and return their sum.
   */
  friend T operator+(const T& lhs, const T& rhs) {
    T temp(lhs);
    return temp += rhs;
  };

  /**
   * @brief Subtract two elements and return their difference.
   */
  friend T operator-(const T& lhs, const T& rhs) {
    T temp(lhs);
    return temp -= rhs;
  };

  /**
   * @brief Return the negation of an element.
   */
  friend T operator-(const T& elem) {
    T temp(elem);
    return temp.Negate();
  };

  /**
   * @brief Multiply two elements and return their product.
   */
  friend T operator*(const T& lhs, const T& rhs) {
    T temp(lhs);
    return temp *= rhs;
  };

  /**
   * @brief Divide two elements and return their quotient.
   */
  friend T operator/(const T& lhs, const T& rhs) {
    T temp(lhs);
    return temp /= rhs;
  };

  /**
   * @brief Compare two elements for equality.
   */
  friend bool operator==(const T& lhs, const T& rhs) {
    return lhs.Equal(rhs);
  };

  /**
   * @brief Compare two elements for inequality.
   */
  friend bool operator!=(const T& lhs, const T& rhs) {
    return !(lhs == rhs);
  };

  /**
   * @brief Write a string representation of an element to a stream.
   */
  friend std::ostream& operator<<(std::ostream& os, const T& r) {
    return os << r.ToString();
  };
};

/**
 * @brief Use to ensure a template parameter is a ring.
 *
 * enable_if_ring will check if its first parameter is a ring (i.e., it inherits
 * from RingElement) and if so, set <code>type</code> to be V. Otherwise it
 * fails to compile.
 */
template <typename T, typename V>
struct EnableIfRing {
  //! type when T is a ring.
  using Type =
      typename std::enable_if<std::is_base_of<Ring<T>, T>::value, V>::type;
};

}  // namespace scl::math

#endif  // SCL_MATH_RING_H
