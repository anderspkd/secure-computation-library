/**
 * @file ff.h
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

#ifndef SCL_MATH_FF_H
#define SCL_MATH_FF_H

#include <algorithm>
#include <string>
#include <type_traits>

#include "scl/math/ff_ops.h"
#include "scl/math/ring.h"
#include "scl/primitives/prg.h"

namespace scl {

/**
 * @brief Elements of the finite field \f$\mathbb{F}\f$.
 *
 * <p>scl::FF defines the interface for finite field elements used throughout
 * SCL. Behaviour of a particular finite field is given through the \p Field
 * template parameter (which acts sort of like a tagging class), combined with
 * appropriate specializations for the functions in ff_ops.h.</p>
 *
 * @tparam Field the finite field.
 */
template <typename Field>
class FF final : details::RingBase<FF<Field>> {
 public:
  /**
   * @brief Size in bytes of a field element.
   */
  constexpr static std::size_t ByteSize() {
    return Field::kByteSize;
  };

  /**
   * @brief Actual bit size of an element.
   */
  constexpr static std::size_t BitSize() {
    return Field::kBitSize;
  };

  /**
   * @brief A short string representation of this field.
   */
  constexpr static const char* Name() {
    return Field::kName;
  };

  /**
   * @brief Read a field element from a buffer.
   * @param src the buffer
   * @return a field element.
   * @see scl::details::FieldFromBytes
   */
  static FF Read(const unsigned char* src) {
    FF e;
    details::FieldFromBytes<Field>(e.mValue, src);
    return e;
  }

  /**
   * @brief Create a random element, using a supplied PRG.
   * @param prg the PRG
   * @return a random element.
   */
  static FF Random(PRG& prg) {
    unsigned char buffer[FF<Field>::ByteSize()];
    prg.Next(buffer, FF<Field>::ByteSize());
    return FF<Field>::Read(buffer);
  };

  /**
   * @brief Create a field element from a string.
   * @param str the string
   * @return a finite field element.
   * @see scl::details::FieldFromString
   */
  static FF FromString(const std::string& str) {
    FF e;
    details::FieldFromString<Field>(e.mValue, str);
    return e;
  };

  /**
   * @brief Get the additive identity of this field.
   */
  static FF Zero() {
    static FF zero;
    return zero;
  };

  /**
   * @brief Get the multiplicative identity of this field.
   */
  static FF One() {
    static FF one(1);
    return one;
  };

  /**
   * @brief Create a new element from an int.
   * @param value the value to interpret as a field element
   * @see scl::details::FieldConvertIn
   */
  explicit constexpr FF(int value) {
    details::FieldConvertIn<Field>(mValue, value);
  };

  /**
   * @brief Create a new element equal to 0 in the field.
   */
  explicit constexpr FF() : FF(0){};

  /**
   * @brief Add another field element to this.
   * @param other the other element
   * @return this set to this + \p other.
   * @see scl::details::FieldAdd
   */
  FF& operator+=(const FF& other) {
    details::FieldAdd<Field>(mValue, other.mValue);
    return *this;
  };

  /**
   * @brief Subtract another field element to this.
   * @param other the other element
   * @return this set to this - \p other.
   * @see scl::details::FieldSubtract
   */
  FF& operator-=(const FF& other) {
    details::FieldSubtract<Field>(mValue, other.mValue);
    return *this;
  };

  /**
   * @brief Multiply another field element to this.
   * @param other the other element
   * @return this set to this * \p other.
   * @see scl::details::FieldMultiply
   */
  FF& operator*=(const FF& other) {
    details::FieldMultiply<Field>(mValue, other.mValue);
    return *this;
  };

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
  };

  /**
   * @brief Negates this element.
   * @return this set to -this.
   * @see scl::details::FieldNegate
   */
  FF& Negate() {
    details::FieldNegate<Field>(mValue);
    return *this;
  };

  /**
   * @brief Computes the additive inverse of this element.
   * @return the additive inverse of this.
   * @see FF::Negate
   */
  FF Negated() const {
    auto copy = mValue;
    FF r;
    details::FieldNegate<Field>(copy);
    r.mValue = copy;
    return r;
  };

  /**
   * @brief Inverts this element.
   * @return this set to its inverse.
   * @see scl::details::FieldInvert
   */
  FF& Invert() {
    details::FieldInvert<Field>(mValue);
    return *this;
  };

  /**
   * @brief Computes the inverse of this element.
   * @return the inverse of this element.
   * @see FF::Invert
   */
  FF Inverse() const {
    FF copy = *this;
    return copy.Invert();
  };

  /**
   * @brief Checks if this element is equal to another.
   * @param other the other element
   * @return true if this is equal to \p other.
   * @see scl::details::FieldEqual
   */
  bool Equal(const FF& other) const {
    return details::FieldEqual<Field>(mValue, other.mValue);
  };

  /**
   * @brief Returns a string representation of this element.
   * @return a string representation of this field element.
   * @see scl::details::FieldToString
   */
  std::string ToString() const {
    return details::FieldToString<Field>(mValue);
  };

  /**
   * @brief Write this element to a byte buffer.
   * @param dest the buffer. Must have space for \ref ByteSize() bytes.
   * @see scl::details::FieldToBytes
   */
  void Write(unsigned char* dest) const {
    details::FieldToBytes<Field>(dest, mValue);
  };

 private:
  typename Field::ValueType mValue;

  template <typename T>
  friend class SCL_FF_Extras;
};

}  // namespace scl

#endif  // SCL_MATH_FF_H
