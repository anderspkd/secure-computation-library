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

#ifndef _SCL_MATH_FF_H
#define _SCL_MATH_FF_H

#include <algorithm>
#include <string>
#include <type_traits>

#include "scl/math/bases.h"
#include "scl/math/fields.h"
#include "scl/math/ring.h"
#include "scl/prg.h"

namespace scl {
namespace details {

/**
 * @brief Elements of the finite field \f$\mathbb{F}_p\f$ for prime \f$p\f$.
 *
 * <p>This class defines finite field element that behaves according to some
 * finite field definition as specified by the \p Field template parameter. The
 * \p Bits parameter is used to indicate the number of available bits of the
 * field, but is otherwise ignored.</p>
 *
 * <p>FF uses will call a number of static methods on \p Field, so these must be
 * defined in order for FF to work. The interface of these methods can be seen
 * in #DEFINE_FINITE_FIELD or by reading the documentation of the methods on
 * FF.</p>
 *
 * @tparam Bits the desired bit size of the field
 * @tparam Field a class defining operations on field elements
 * @see DEFINE_FINITE_FIELD
 */
template <unsigned Bits, typename Field>
class FF final : details::RingBase<FF<Bits, Field>> {
 public:
  /**
   * @brief the raw type of a finite field element.
   *
   * The internal/raw type of a field element is read from
   * <code>Field::ValueType</code>.
   */
  using ValueType = typename Field::ValueType;

  /**
   * @brief Size of the field as specified in the \p Bits template parameter.
   */
  constexpr static std::size_t SpecifiedBitSize() { return Bits; };

  /**
   * @brief Size in bytes of a field element.
   *
   * The byte size of field element is defined as
   * <code>Field::kByteSize</code>. For pre-defined fields in SCL, this value
   * will be equal to <code>sizeof(ValueType)</code>.
   */
  constexpr static std::size_t ByteSize() { return Field::kByteSize; };

  /**
   * @brief Actual bit size of an element.
   *
   * The bit-size of a field element is defined as <code>Field::kBitSize</code>.
   */
  constexpr static std::size_t BitSize() { return Field::kBitSize; };

  /**
   * @brief A short string representation of this field.
   *
   * The name of finite field instantiation is read from the constant
   * <code>Field::kName</code>.
   */
  constexpr static const char* Name() { return Field::kName; };

  /**
   * @brief Read a field element from a buffer.
   *
   * The de-serialization of field elements are defined by a static method
   * <code>Field::FromBytes(ValueType&, const unsigned char*)</code>.
   *
   * @param src the buffer
   * @return a field element.
   */
  static FF Read(const unsigned char* src) {
    FF e;
    Field::FromBytes(e.mValue, src);
    return e;
  }

  /**
   * @brief Create a random element, using a supplied PRG.
   * @param prg the PRG
   * @return a random element.
   */
  static FF Random(PRG& prg) {
    unsigned char buffer[FF<Bits, Field>::ByteSize()];
    prg.Next(buffer, FF<Bits, Field>::ByteSize());
    return FF<Bits, Field>::Read(buffer);
  };

  /**
   * @brief Create a field element from a string.
   *
   * De-stringification is determined by the static method
   * <code>Field::FromString(ValueType&, const std::string&, enum
   * NumberBase)</code>.
   *
   * @param str the string
   * @param base the base of the string
   * @return a finite field element.
   */
  static FF FromString(const std::string& str, enum NumberBase base) {
    FF e;
    Field::FromString(e.mValue, str, base);
    return e;
  };

  /**
   * @brief Create a field element from a base 10 string.
   * @param str the string
   * @return a finite field element.
   * @see FF::FromString
   */
  static FF FromString(const std::string& str) {
    return FF::FromString(str, NumberBase::DECIMAL);
  };

  /**
   * @brief Create a new element from an int.
   *
   * The resulting field element is created according to the static method
   * <code>Field::FromInt(int)</code>.
   *
   * @param value the value to interpret as a field element
   */
  explicit constexpr FF(int value) : mValue(Field::FromInt(value)){};

  /**
   * @brief Create a new element equal to 0 in the field.
   */
  explicit constexpr FF() : FF(0){};

  /**
   * @brief Add another field element to this.
   *
   * Field element addition is defined by the static method
   * <code>Field::Add(ValueType&, const ValueType&)</code>.
   *
   * @param other the other element
   * @return this set to this + \p other.
   */
  FF& operator+=(const FF& other) {
    Field::Add(mValue, other.mValue);
    return *this;
  };

  /**
   * @brief Subtract another field element to this.
   *
   * Field element subtraction is defined by the static method
   * <code>Field::Subtract(ValueType&, const ValueType&)</code>.
   *
   * @param other the other element
   * @return this set to this - \p other.
   */
  FF& operator-=(const FF& other) {
    Field::Subtract(mValue, other.mValue);
    return *this;
  };

  /**
   * @brief Multiply another field element to this.
   *
   * Field element multiplication is defined by the static method
   * <code>Field::Multiply(ValueType&, const ValueType&)</code>.
   *
   * @param other the other element
   * @return this set to this * \p other.
   */
  FF& operator*=(const FF& other) {
    Field::Multiply(mValue, other.mValue);
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
  FF& operator/=(const FF& other) { return operator*=(other.Inverse()); };

  /**
   * @brief Negates this element.
   *
   * Field element negation is defined by
   * <code>Field::Negate(ValueType&)</code>.
   *
   * @return this set to -this.
   */
  FF& Negate() {
    Field::Negate(mValue);
    return *this;
  };

  /**
   * @brief Computes the additive inverse of this element.
   * @return the additive inverse of this.
   * @see FF::Negate
   */
  FF Negated() {
    auto copy = mValue;
    FF r;
    Field::Negate(copy);
    r.mValue = copy;
    return r;
  };

  /**
   * @brief Inverts this element.
   *
   * Computation of field element inverses is defined by
   * <code>Field::Invert(ValueType)</code>.
   *
   * @return this set to its inverse.
   */
  FF& Invert() {
    Field::Invert(mValue);
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
   */
  bool Equal(const FF& other) const {
    return Field::Equal(mValue, other.mValue);
  };

  /**
   * @brief Returns a string representation of this element.
   *
   * Stringification of an element is defined by
   * <code>Field::ToString(ValueType)</code>.
   *
   * @return a string representation of this field element.
   */
  std::string ToString() const { return Field::ToString(mValue); };

  /**
   * @brief Write this element to a byte buffer.
   *
   * <p>Serializes this field element. Serialization is defined by
   * <code>Field::ToBytes(unsigned char*, const ValueType&)</code>. In general,
   * it should be the case that \p dest has space for \ref ByteSize() amount of
   * bytes, although more may be needed depending on Field.</p>
   *
   * <p>For pre-defined fields in SCL, \p dest <i>must</i> have space for \ref
   * ByteSize() amount of bytes.</p>
   *
   * @param dest the buffer. Must have space for \ref ByteSize() bytes.
   */
  void Write(unsigned char* dest) const { Field::ToBytes(dest, mValue); }

 private:
  ValueType mValue;
};

}  // namespace details

/**
 * @brief A suitable pre-defined field with space for \p Bits computation.
 *
 * <p>scl::FF picks a suitable pre-defined finite field implementation based on
 * the number of bits of computation the user wants. At the moment, there are
 * two different fields supported: One based on a 61-bit Mersenne prime, and one
 * based on a 127-bit mersenne prime. Depending on \p Bits, the smaller of the
 * fields will be picked and a compile-time error is thrown if \p Bits exceed
 * 127.</p>
 */
template <unsigned Bits>
using FF = details::FF<Bits, typename details::FieldSelector<Bits>::Field>;

}  // namespace scl

#endif  // _SCL_MATH_FF_H
