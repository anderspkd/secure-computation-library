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
#include "scl/math/field_ops.h"
#include "scl/math/ring.h"
#include "scl/prg.h"

namespace scl {
namespace details {

/**
 * @brief Elements of the finite field \f$\mathbb{F}\f$.
 *
 * <p>The FF class provides the high level interface for "finite field elements"
 * in SCL.</p>
 *
 * <p>An instantiation of this template consists of two parameters: A \p Bits
 * parameter, indicating the number of bits that the field supports, and a \p
 * Field type which determines the behaviour of the field. The \p Bits parameter
 * is purely for documenting purposes and can be used to indicate that a
 * particular field instantiation supports a particular number of bits for
 * computing.</p>
 *
 * <p>The \p Field parameter must be a class of the following form</p>
 *
 * \code
 *   struct SomeField {
 *     using ValueType = ... // the internal type of a field element
 *     constexpr static const char* kName = ... // readable name of this field
 *     constexpr static const std::size_t kByteSize = ... // size in bytes
 *     constexpr static const std::size_t kBitSize = ... // size in bits
 *   };
 * \endcode
 *
 * <p>The <code>kBitSize</code> does not need to have any relation to the \p
 * Bits parameter, although it's assumed that <code>Bits <= kBitSize</code>.</p>
 *
 * <p>In order to make a field definition useful, overloads must be defined for
 * some or all of the functions in field_ops.h. For example, to support addition
 * operations on the field (that is, the operators <code>+</code> and
 * <code>+=</code>) we would need to define</p>
 *
 * \code
 *   template <>
 *   void scl::details::FieldAdd<SomeField>(SomeField::ValueType& out,
 *                                          const SomeField::ValueType& in) {
 *     // definition of addition for SomeField
 *   }
 * \endcode
 *
 * <p>Refer to the documentation for methods on FF to see which specializations
 * are needed where.</p>
 *
 * @tparam Bits the desired bit size of the field
 * @tparam Field the field
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
    unsigned char buffer[FF<Bits, Field>::ByteSize()];
    prg.Next(buffer, FF<Bits, Field>::ByteSize());
    return FF<Bits, Field>::Read(buffer);
  };

  /**
   * @brief Create a field element from a string.
   * @param str the string
   * @param base the base of the string
   * @return a finite field element.
   * @see scl::details::FieldFromString
   */
  static FF FromString(const std::string& str, enum NumberBase base) {
    FF e;
    details::FieldFromString<Field>(e.mValue, str, base);
    return e;
  };

  /**
   * @brief Create a field element from a base 10 string.
   * @param str the string
   * @return a finite field element.
   */
  static FF FromString(const std::string& str) {
    return FF::FromString(str, NumberBase::DECIMAL);
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
  FF& operator/=(const FF& other) { return operator*=(other.Inverse()); };

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
  FF Negated() {
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
  ValueType mValue;
};

/**
 * @brief The field \f$\mathbb{F}_p\f$ with \f$p=2^{61}-1\f$.
 */
struct Mersenne61 {
  /**
   * @brief Internal type elements of this field.
   */
  using ValueType = std::uint64_t;

  /**
   * @brief The name of this field.
   */
  constexpr static const char* kName = "Mersenne61";

  /**
   * @brief The size of field elements of this field in bytes.
   */
  constexpr static const std::size_t kByteSize = sizeof(ValueType);

  /**
   * @brief The size of field elements of this field in bits.
   */
  constexpr static const std::size_t kBitSize = 61;
};

/**
 * @brief The field \f$\mathbb{F}_p\f$ with \f$p=2^{127}-1\f$.
 */
struct Mersenne127 {
  /**
   * @brief Internal type elements of this field.
   */
  using ValueType = __uint128_t;

  /**
   * @brief The name of this field.
   */
  constexpr static const char* kName = "Mersenne127";

  /**
   * @brief The size of field elements of this field in bytes.
   */
  constexpr static const std::size_t kByteSize = sizeof(ValueType);

  /**
   * @brief The size of field elements of this field in bits.
   */
  constexpr static const std::size_t kBitSize = 127;
};

#define _SCL_LE(a, b) ((a) <= (b))
#define _SCL_GE(a, b) ((a) >= (b))

/**
 * @brief Select a suitable Finite Field based on a provided bitlevel.
 */
template <unsigned Bits>
struct FieldSelector {
  /**
   * @brief The field.
   */
  // clang-format off
  using Field =
      std::conditional_t<
      _SCL_GE(Bits, 1) && _SCL_LE(Bits, 61),
      Mersenne61,

      std::conditional_t<
      _SCL_GE(Bits, 62) && _SCL_LE(Bits, 127),
      Mersenne127,

      void>>;

  // clang-format on
};

#undef _SCL_LE
#undef _SCL_GE

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
