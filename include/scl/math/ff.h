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

#ifndef SCL_MATH_FF_H
#define SCL_MATH_FF_H

#include <algorithm>
#include <string>
#include <type_traits>

#include "scl/math/fields/ff_ops.h"
#include "scl/serialization/serializer.h"
#include "scl/util/prg.h"

namespace scl {
namespace math {

/**
 * @brief Finite Field interface.
 * @tparam FIELD finite field definition.
 */
template <typename FIELD>
class FF final {
 public:
  /**
   * @brief Size in bytes of a field element.
   */
  constexpr static std::size_t byteSize() {
    return FIELD::BYTE_SIZE;
  }

  /**
   * @brief Actual bit size of an element.
   */
  constexpr static std::size_t bitSize() {
    return FIELD::BIT_SIZE;
  }

  /**
   * @brief A short string representation of this field.
   */
  constexpr static const char* name() {
    return FIELD::NAME;
  }

  /**
   * @brief Read a field element from a buffer.
   */
  static FF read(const unsigned char* src) {
    FF e;
    ff::fromBytes<FIELD>(e.m_value, src);
    return e;
  }  // LCOV_EXCL_LINE

  /**
   * @brief Create a random element, using a supplied PRG.
   */
  static FF random(util::PRG& prg) {
    unsigned char buffer[FF<FIELD>::byteSize()];
    prg.next(buffer, FF<FIELD>::byteSize());
    return FF<FIELD>::read(buffer);
  }

  /**
   * @brief Create a field element from a hex-string.
   */
  static FF fromString(const std::string& hexstr) {
    FF e;
    ff::convertTo<FIELD>(e.m_value, hexstr);
    return e;
  }

  /**
   * @brief Get the additive identity of this field.
   */
  static FF zero() {
    static FF zero;
    return zero;
  }

  /**
   * @brief Get the multiplicative identity of this field.
   */
  static FF one() {
    static FF one(1);
    return one;
  }

  /**
   * @brief Create a new element from an int.
   */
  explicit constexpr FF(int value) {
    ff::convertTo<FIELD>(m_value, value);
  }

  /**
   * @brief Create a new element equal to 0 in the field.
   */
  constexpr FF() : FF(0) {}

  /**
   * @brief Destrutor. Does nothing.
   */
  ~FF() {}

  /**
   * @brief Add another field element to this.
   */
  FF& operator+=(const FF& other) {
    ff::add<FIELD>(m_value, other.m_value);
    return *this;
  }

  /**
   * @brief Add two finite field elements.
   */
  friend FF operator+(const FF& lhs, const FF& rhs) {
    FF tmp(lhs);
    return tmp += rhs;
  }

  /**
   * @brief Pre-increment this finite-field element.
   */
  FF& operator++() {
    return *this += one();
  }

  /**
   * @brief Post-increment this finite-field element.
   */
  friend FF operator++(FF& e, int) {
    FF tmp(e);
    ++e;
    return tmp;
  }

  /**
   * @brief Subtract another field element to this.
   */
  FF& operator-=(const FF& other) {
    ff::subtract<FIELD>(m_value, other.m_value);
    return *this;
  }

  /**
   * @brief Subtract two finite field elements.
   */
  friend FF operator-(const FF& lhs, const FF& rhs) {
    FF tmp(lhs);
    return tmp -= rhs;
  }

  /**
   * @brief Pre-decrement this finite field element.
   */
  FF& operator--() {
    return *this -= one();
  }

  /**
   * @brief Post-decrement this finite field element.
   */
  friend FF operator--(FF& e, int) {
    FF tmp(e);
    --e;
    return tmp;
  }

  /**
   * @brief Multiply another field element to this.
   */
  FF& operator*=(const FF& other) {
    ff::multiply<FIELD>(m_value, other.m_value);
    return *this;
  }

  /**
   * @brief Multiply two finite field elements.
   */
  friend FF operator*(const FF& lhs, const FF& rhs) {
    FF tmp(lhs);
    return tmp *= rhs;
  }

  /**
   * @brief Divide and assign this finite field element with another element.
   */
  FF& operator/=(const FF& other) {
    return operator*=(other.inverse());
  }

  /**
   * @brief Divide two finite field elements.
   */
  friend FF operator/(const FF& lhs, const FF& rhs) {
    FF tmp(lhs);
    return tmp /= rhs;
  }

  /**
   * @brief Negates this element.
   */
  FF& negate() {
    ff::negate<FIELD>(m_value);
    return *this;
  }

  /**
   * @brief Computes the additive inverse of this element.
   */
  FF negated() const {
    auto copy = m_value;
    FF r;
    ff::negate<FIELD>(copy);
    r.m_value = copy;
    return r;
  }  // LCOV_EXCL_LINE

  /**
   * @brief Negate a finite field element.
   */
  friend FF operator-(const FF& e) {
    return e.negated();
  }

  /**
   * @brief Inverts this element.
   */
  FF& invert() {
    ff::invert<FIELD>(m_value);
    return *this;
  }

  /**
   * @brief Computes the inverse of this element.
   */
  FF inverse() const {
    FF copy = *this;
    return copy.invert();
  }

  /**
   * @brief Checks if this element is equal to another.
   */
  bool equal(const FF& other) const {
    return ff::equal<FIELD>(m_value, other.m_value);
  }

  /**
   * @brief Equality operator for finite field elements.
   */
  friend bool operator==(const FF& lhs, const FF& rhs) {
    return lhs.equal(rhs);
  }

  /**
   * @brief In-equality operator for finite field elements.
   */
  friend bool operator!=(const FF& lhs, const FF& rhs) {
    return !(lhs == rhs);
  }

  /**
   * @brief Returns a string representation of this element.
   */
  std::string toString() const {
    return ff::toString<FIELD>(m_value);
  }

  /**
   * @brief Write a string representation of a finite field element to a stream.
   */
  friend std::ostream& operator<<(std::ostream& os, const FF& e) {
    return os << e.toString();
  }

  /**
   * @brief Write this element to a byte buffer.
   */
  void write(unsigned char* dest) const {
    ff::toBytes<FIELD>(dest, m_value);
  }

  /**
   * @brief Get the internal value of this field element.
   */
  typename FIELD::ValueType value() const {
    return m_value;
  }

  /**
   * @brief Get the internal value of this field element.
   */
  typename FIELD::ValueType& value() {
    return m_value;
  }

 private:
  typename FIELD::ValueType m_value;
};

/**
 * @brief Returns the order of a finite field.
 */
template <typename F>
Number order();

/**
 * @brief Raise an element to a power.
 * @param base the base.
 * @param exp the exponent.
 * @return \p base raised to the \p exp th power.
 */
template <typename T>
FF<T> exp(const FF<T>& base, std::size_t exp) {
  FF r = FF<T>::one();

  if (exp == 0) {
    return r;
  }

  const auto n = sizeof(std::size_t) * 8 - __builtin_clzll(exp);
  for (std::size_t i = n; i-- > 0;) {
    r *= r;
    if (((exp >> i) & 1) == 1) {
      r *= base;
    }
  }

  return r;
}

}  // namespace math

namespace seri {

/**
 * @brief Serializer specialization for math::FF types.
 */
template <typename FIELD>
struct Serializer<math::FF<FIELD>> {
  /**
   * @brief Determine the size of an math::FF value.
   *
   * The size of an math::FF element can be determined from its type alone, so
   * the argument is ignored.
   */
  static constexpr std::size_t sizeOf(const math::FF<FIELD>& /* ignored */) {
    return math::FF<FIELD>::byteSize();
  }

  /**
   * @brief Write an math::FF element to a buffer.
   * @param elem the element.
   * @param buf the buffer.
   *
   * Calls math::FF::write().
   */
  static std::size_t write(const math::FF<FIELD>& elem, unsigned char* buf) {
    elem.write(buf);
    return sizeOf(elem);
  }

  /**
   * @brief Read an math::FF element from a buffer.
   * @param elem output variable holding the read element after reading.
   * @param buf the buffer.
   * @return the number of bytes read.
   *
   * Calls math::FF::read() and returns math::FF::byteSize();
   */
  static std::size_t read(math::FF<FIELD>& elem, const unsigned char* buf) {
    elem = math::FF<FIELD>::read(buf);
    return sizeOf(elem);
  }
};

}  // namespace seri
}  // namespace scl

#endif  // SCL_MATH_FF_H
