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

#ifndef SCL_MATH_FF_H
#define SCL_MATH_FF_H

#include <algorithm>
#include <string>
#include <type_traits>

#include "scl/math/ff_ops.h"
#include "scl/math/ring.h"
#include "scl/util/prg.h"

namespace scl::math {

/**
 * @brief Finite Field interface.
 * @tparam Field finite field definition
 * @see Mersenne61
 * @see Mersenne127
 */
template <typename Field>
class FF final : Ring<FF<Field>> {
 public:
  /**
   * @brief Size in bytes of a field element.
   */
  constexpr static std::size_t ByteSize() {
    return Field::BYTE_SIZE;
  }

  /**
   * @brief Actual bit size of an element.
   */
  constexpr static std::size_t BitSize() {
    return Field::BIT_SIZE;
  }

  /**
   * @brief A short string representation of this field.
   */
  constexpr static const char* Name() {
    return Field::NAME;
  }

  /**
   * @brief Read a field element from a buffer.
   * @param src the buffer
   * @return a field element.
   * @see scl::FieldFromBytes
   */
  static FF Read(const unsigned char* src) {
    FF e;
    FieldFromBytes<Field>(e.m_value, src);
    return e;
  }  // LCOV_EXCL_LINE

  /**
   * @brief Create a random element, using a supplied PRG.
   * @param prg the PRG
   * @return a random element.
   */
  static FF Random(util::PRG& prg) {
    unsigned char buffer[FF<Field>::ByteSize()];
    prg.Next(buffer, FF<Field>::ByteSize());
    return FF<Field>::Read(buffer);
  }

  /**
   * @brief Create a field element from a string.
   * @param str the string
   * @return a finite field element.
   * @see scl::FieldFromString
   */
  static FF FromString(const std::string& str) {
    FF e;
    FieldFromString<Field>(e.m_value, str);
    return e;
  }

  /**
   * @brief Get the additive identity of this field.
   */
  static FF Zero() {
    static FF zero;
    return zero;
  }

  /**
   * @brief Get the multiplicative identity of this field.
   */
  static FF One() {
    static FF one(1);
    return one;
  }

  /**
   * @brief Create a new element from an int.
   * @param value the value to interpret as a field element
   * @see scl::FieldConvertIn
   */
  explicit constexpr FF(int value) {
    FieldConvertIn<Field>(m_value, value);
  }

  /**
   * @brief Create a new element equal to 0 in the field.
   */
  explicit constexpr FF() : FF(0) {}

  /**
   * @brief Destrutor. Does nothing.
   */
  ~FF() {}

  /**
   * @brief Add another field element to this.
   * @param other the other element
   * @return this set to this + \p other.
   * @see scl::FieldAdd
   */
  FF& operator+=(const FF& other) {
    FieldAdd<Field>(m_value, other.m_value);
    return *this;
  }

  /**
   * @brief Subtract another field element to this.
   * @param other the other element
   * @return this set to this - \p other.
   * @see scl::FieldSubtract
   */
  FF& operator-=(const FF& other) {
    FieldSubtract<Field>(m_value, other.m_value);
    return *this;
  }

  /**
   * @brief Multiply another field element to this.
   * @param other the other element
   * @return this set to this * \p other.
   * @see scl::FieldMultiply
   */
  FF& operator*=(const FF& other) {
    FieldMultiply<Field>(m_value, other.m_value);
    return *this;
  }

  /**
   * @brief Multiplies this with the inverse of another elemenet.
   *
   * This is not an integer division. Rather, it computes \f$x\cdot y^{-1}\f$
   * where \f$x\f$ is this element and \f$y\f$ is \p other.
   *
   * @param other the other element
   * @return this set to this * <code>other.Inverse()</code>.
   */
  FF& operator/=(const FF& other) {
    return operator*=(other.Inverse());
  }

  /**
   * @brief Negates this element.
   * @return this set to -this.
   * @see scl::FieldNegate
   */
  FF& Negate() {
    FieldNegate<Field>(m_value);
    return *this;
  }

  /**
   * @brief Computes the additive inverse of this element.
   * @return the additive inverse of this.
   * @see FF::Negate
   */
  FF Negated() const {
    auto copy = m_value;
    FF r;
    FieldNegate<Field>(copy);
    r.m_value = copy;
    return r;
  }  // LCOV_EXCL_LINE

  /**
   * @brief Inverts this element.
   * @return this set to its inverse.
   * @see scl::FieldInvert
   */
  FF& Invert() {
    FieldInvert<Field>(m_value);
    return *this;
  }

  /**
   * @brief Computes the inverse of this element.
   * @return the inverse of this element.
   * @see FF::Invert
   */
  FF Inverse() const {
    FF copy = *this;
    return copy.Invert();
  }

  /**
   * @brief Checks if this element is equal to another.
   * @param other the other element
   * @return true if this is equal to \p other.
   * @see scl::FieldEqual
   */
  bool Equal(const FF& other) const {
    return FieldEqual<Field>(m_value, other.m_value);
  }

  /**
   * @brief Returns a string representation of this element.
   * @return a string representation of this field element.
   * @see scl::FieldToString
   */
  std::string ToString() const {
    return FieldToString<Field>(m_value);
  }

  /**
   * @brief Write this element to a byte buffer.
   * @param dest the buffer. Must have space for \ref ByteSize() bytes.
   * @see scl::FieldToBytes
   */
  void Write(unsigned char* dest) const {
    FieldToBytes<Field>(dest, m_value);
  }

 private:
  typename Field::ValueType m_value;

  template <typename T>
  friend class FFAccess;
};

}  // namespace scl::math

#endif  // SCL_MATH_FF_H
