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

#ifndef SCL_MATH_FIELDS_NAF_H
#define SCL_MATH_FIELDS_NAF_H

#include <array>
#include <cstddef>

#include "scl/math/ff.h"

namespace scl::math::details {

/**
 * @brief Non-adjacent Form encoding of a field element.
 * @tparam T a finite field type.
 */
template <typename T>
struct NafEncoding {
 public:
  /**
   * @brief A type indicating the sign in the encoding.
   */
  class Value {
   public:
    constexpr Value() : Value(0) {}

    /**
     * @brief Create a value representing +1.
     */
    static constexpr Value createPos() {
      return Value{1};
    }

    /**
     * @brief Create a value representing -1.
     */
    static constexpr Value createNeg() {
      return Value{2};
    }

    /**
     * @brief Create a value represeting 0.
     */
    static constexpr Value createZero() {
      return Value{0};
    }

    /**
     * @brief Check if this value is +1.
     */
    constexpr bool pos() const {
      return m_v == 1;
    }

    /**
     * @brief Check if this value is -1.
     */
    constexpr bool neg() const {
      return m_v == 2;
    }

    /**
     * @brief Check if this value is 0.
     */
    constexpr bool zero() const {
      return m_v == 0;
    }

   private:
    constexpr Value(unsigned char v) : m_v(v) {}
    unsigned char m_v;
  };

  /**
   * @brief Maximum size of the encoding.
   */
  constexpr static std::size_t MAX_SIZE = T::BIT_SIZE + 1;

  /**
   * @brief The trits.
   */
  std::array<Value, MAX_SIZE> values;

  /**
   * @brief The number of meaningful entries in <code>values</code>.
   */
  std::size_t size;
};

}  // namespace scl::math::details

#endif  // SCL_MATH_FIELDS_NAF_H
