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

#ifndef SCL_MATH_Z2K_H
#define SCL_MATH_Z2K_H

#include <cstdint>
#include <stdexcept>

#include "scl/math/z2k/z2k_ops.h"
#include "scl/util/prg.h"

namespace scl::math {

/**
 * @brief Elements of the ring \f$\mathbb{Z}_{2^K}\f$ for integer \f$K\f$.
 * @tparam Bits the size of the ring.
 *
 * This class defines the ring of integers modulo a power of two. The bitsize
 * specified in the template parameter corresponds to the power of two
 * used. When elements of Z2k are serialized, they are padded to the nearest
 * byte (so Z2k<6> and Z2k<8> take up the same amount of space).
 */
template <std::size_t BITS>
class Z2k final {
 public:
  /**
   * @brief The raw type of a Z2k element.
   */
  using ValueType =
      std::conditional_t<(BITS <= 64), std::uint64_t, __uint128_t>;

  /**
   * @brief The number of bytes needed to store a ring element.
   */
  constexpr static std::size_t byteSize() {
    return (BITS - 1) / 8 + 1;
  }

  /**
   * @brief The bit size of the ring.
   */
  constexpr static std::size_t bitSize() {
    return BITS;
  }

  /**
   * @brief A short string representation of this ring.
   */
  constexpr static const char* name() {
    return "Z2k";
  }

  /**
   * @brief Read a Z2k element from a buffer.
   */
  static Z2k read(const unsigned char* src) {
    Z2k e;
    z2k::fromBytes<ValueType, bitSize()>(e.m_value, src);
    return e;
  }

  /**
   * @brief Create a random element.
   */
  static Z2k random(util::PRG& prg) {
    unsigned char buffer[byteSize()];
    prg.next(buffer, byteSize());
    return Z2k::read(buffer);
  }

  /**
   * @brief Create a ring element from a string.
   */
  static Z2k fromString(const std::string& str) {
    Z2k e;
    z2k::convertIn<ValueType, bitSize()>(e.m_value, str);
    return e;
  }  // LCOV_EXCL_LINE

  /**
   * @brief Get the additive identity of this ring.
   */
  static Z2k zero() {
    static Z2k zero;
    return zero;
  }

  /**
   * @brief Get the multiplicative identity of this ring.
   */
  static Z2k one() {
    static Z2k one(1);
    return one;
  }

  /**
   * @brief Create a new ring element from a value.
   */
  explicit constexpr Z2k(const ValueType& value) : m_value(value) {}

  /**
   * @brief Create a new ring element equal to 0.
   */
  constexpr Z2k() : m_value(0) {}

  /**
   * @brief Destructor. Does nothing.
   */
  ~Z2k() {}

  /**
   * @brief Add another element to this.
   */
  Z2k& operator+=(const Z2k& other) {
    z2k::add(m_value, other.m_value);
    return *this;
  }

  /**
   * @brief Add two Z2k elements together.
   */
  friend Z2k operator+(const Z2k& lhs, const Z2k& rhs) {
    Z2k tmp(lhs);
    return tmp += rhs;
  }

  /**
   * @brief Pre-increment this element.
   */
  Z2k& operator++() {
    return *this += one();
  }

  /**
   * @brief Post-increment this element.
   */
  friend Z2k operator++(Z2k& e, int) {
    Z2k tmp(e);
    ++e;
    return tmp;
  }

  /**
   * @brief Subtract another element from this.
   */
  Z2k& operator-=(const Z2k& other) {
    z2k::subtract(m_value, other.m_value);
    return *this;
  }

  /**
   * @brief Subtract two Z2k elements from each other.
   */
  friend Z2k operator-(const Z2k& lhs, const Z2k& rhs) {
    Z2k tmp(lhs);
    return tmp -= rhs;
  }

  /**
   * @brief Pre-decrement this element.
   */
  Z2k& operator--() {
    return *this -= one();
  }

  /**
   * @brief Post-decrement this element.
   */
  friend Z2k operator--(Z2k& e, int) {
    Z2k tmp(e);
    --e;
    return tmp;
  }

  /**
   * @brief Multiply another element to this.
   */
  Z2k& operator*=(const Z2k& other) {
    z2k::multiply(m_value, other.m_value);
    return *this;
  }

  /**
   * @brief Multiply two Z2k elements together.
   */
  friend Z2k operator*(const Z2k& lhs, const Z2k& rhs) {
    Z2k tmp(lhs);
    return tmp *= rhs;
  }

  /**
   * @brief Divide this element by another.
   * @throws std::invalid_argument if \p other is not invertible.
   */
  Z2k& operator/=(const Z2k& other) {
    z2k::multiply(m_value, other.inverse().m_value);
    return *this;
  }

  /**
   * @brief Divide two Z2k elements.
   * @throws std::invalid_argument if \p other is not invertible.
   */
  friend Z2k operator/(const Z2k& lhs, const Z2k& rhs) {
    Z2k tmp(lhs);
    return tmp /= rhs;
  }

  /**
   * @brief Negates this element.
   */
  Z2k& negate() {
    z2k::negate(m_value);
    return *this;
  }

  /**
   * @brief Compute the negation of this element.
   */
  Z2k negated() const {
    Z2k copy(m_value);
    return copy.negate();
  }

  /**
   * @brief Negate a Z2k element.
   */
  friend Z2k operator-(const Z2k& e) {
    return e.negated();
  }

  /**
   * @brief Inverts this element.
   * @throws std::invalid_argument if this element is not invertible.
   */
  Z2k& invert() {
    z2k::invert<ValueType, bitSize()>(m_value);
    return *this;
  }

  /**
   * @brief Compute the inverse of this element.
   * @throws std::invalid_argument if this element is not invertible.
   */
  Z2k inverse() const {
    Z2k copy(m_value);
    return copy.invert();
  }

  /**
   * @brief Return the least significant bit of this element.
   */
  unsigned lsb() const {
    return z2k::lsb(m_value);
  }

  /**
   * @brief Check if this element is equal to another element.
   */
  bool equal(const Z2k& other) const {
    return z2k::equal<ValueType, BITS>(m_value, other.m_value);
  }

  /**
   * @brief Equality operator for Z2k elements.
   */
  friend bool operator==(const Z2k& lhs, const Z2k& rhs) {
    return lhs.equal(rhs);
  }

  /**
   * @brief In-equality operator for Z2k elements.
   */
  friend bool operator!=(const Z2k& lhs, const Z2k& rhs) {
    return !(lhs == rhs);
  }

  /**
   * @brief Return a string representation of this element.
   */
  std::string toString() const {
    return z2k::toString<ValueType, bitSize()>(m_value);
  }

  /**
   * @brief Write a string representation of this element to stream.
   */
  friend std::ostream& operator<<(std::ostream& os, const Z2k& e) {
    return os << e.toString();
  }

  /**
   * @brief Write this element to a buffer.
   */
  void write(unsigned char* dest) const {
    z2k::toBytes<ValueType, bitSize()>(m_value, dest);
  }

 private:
  ValueType m_value;
};

}  // namespace scl::math

#endif  // SCL_MATH_Z2K_H
