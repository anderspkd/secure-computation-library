/**
 * @file z2k.h
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

#ifndef SCL_MATH_Z2K_H
#define SCL_MATH_Z2K_H

#include <stdexcept>

#include "scl/math/ring.h"
#include "scl/math/z2k_ops.h"
#include "scl/primitives/prg.h"

namespace scl {

/**
 * @brief Elements of the ring \f$\mathbb{Z}_{2^K}\f$ for integer \f$K\f$.
 *
 * This class defines the ring of integers modulo a power of two. The bitsize
 * specified in the template parameter corresponds to the power of two
 * used. When elements of Z2k are serialized, they are padded to the nearest
 * byte (so Z2k<6> and Z2k<8> take up the same amount of space).
 *
 * @tparam K the size of the ring.
 */
template <std::size_t K>
class Z2k final : public details::RingBase<Z2k<K>> {
 public:
  /**
   * @brief The raw type of a Z2k element.
   */
  using ValueType = std::conditional_t<(K <= 64), std::uint64_t, __uint128_t>;

  /**
   * @brief The bit size of the ring. Identical to BitSize().
   */
  constexpr static std::size_t SpecifiedBitSize() {
    return K;
  };

  /**
   * @brief The number of bytes needed to store a ring element.
   */
  constexpr static std::size_t ByteSize() {
    return (K - 1) / 8 + 1;
  };

  /**
   * @brief The bit size of the ring. Identical to SpecifiedBitSize().
   */
  constexpr static std::size_t BitSize() {
    return SpecifiedBitSize();
  };

  /**
   * @brief A short string representation of this ring.
   */
  constexpr static const char* Name() {
    return "Z2k";
  };

  /**
   * @brief Read a ring from a buffer.
   * @param src the buffer
   * @return a ring element.
   * @note This method reads exactly ByteSize() bytes of \p src.
   */
  static Z2k Read(const unsigned char* src) {
    Z2k e;
    details::ReadZ2k<ValueType, BitSize()>(e.mValue, src);
    return e;
  };

  /**
   * @brief Create a random element.
   * @param prg a prg used to generate the random element
   * @return a random element.
   */
  static Z2k Random(PRG& prg) {
    unsigned char buffer[ByteSize()];
    prg.Next(buffer, ByteSize());
    return Z2k::Read(buffer);
  };

  /**
   * @brief Create a ring element from a string.
   * @param str the string
   * @return a ring element.
   */
  static Z2k FromString(const std::string& str) {
    Z2k e;
    details::FromStringZ2k<ValueType, BitSize()>(e.mValue, str);
    return e;
  };

  /**
   * @brief Create a new ring element from a value.
   * @param value the value
   */
  explicit constexpr Z2k(const ValueType& value) : mValue(value){};

  /**
   * @brief Create a new ring element equal to 0.
   */
  explicit constexpr Z2k() : mValue(0){};

  /**
   * @brief Add another element to this.
   * @param other the other element
   * @return this incremented by \p other.
   */
  Z2k& operator+=(const Z2k& other) {
    details::AddZ2k(mValue, other.mValue);
    return *this;
  };

  /**
   * @brief Subtract another element from this.
   * @param other the other element
   * @return this decremented by \p other.
   */
  Z2k& operator-=(const Z2k& other) {
    details::SubtractZ2k(mValue, other.mValue);
    return *this;
  };

  /**
   * @brief Multiply another element to this.
   * @param other the other element
   * @return this scaled by \p other.
   */
  Z2k& operator*=(const Z2k& other) {
    details::MultiplyZ2k(mValue, other.mValue);
    return *this;
  };

  /**
   * @brief Divide this element by another.
   * @param other the other element
   * @return this element set to this / \p other.
   * @throws std::invalid_argument if \p other is not invertible.
   */
  Z2k& operator/=(const Z2k& other) {
    details::MultiplyZ2k(mValue, other.Inverse().mValue);
    return *this;
  };

  /**
   * @brief Negates this element.
   */
  Z2k& Negate() {
    details::NegateZ2k(mValue);
    return *this;
  };

  /**
   * @brief Compute the negation of this element.
   */
  Z2k Negated() const {
    Z2k copy(mValue);
    return copy.Negate();
  };

  /**
   * @brief Inverts this element.
   * @throws std::invalid_argument if this element is not invertible.
   */
  Z2k& Invert() {
    details::InvertZ2k<ValueType, BitSize()>(mValue);
    return *this;
  };

  /**
   * @brief Compute the inverse of this element.
   */
  Z2k Inverse() const {
    Z2k copy(mValue);
    return copy.Invert();
  };

  /**
   * @brief Return the least significant bit of this element.
   *
   * This value can be used to determine if an element is invertible or not. In
   * particular, an element <code>x</code> is invertible if <code>x.Lsb() ==
   * 1</code>. That is, if it is odd.
   */
  unsigned Lsb() const {
    return details::LsbZ2k(mValue);
  };

  /**
   * @brief Check if this element is equal to another element.
   */
  bool Equal(const Z2k& other) const {
    return details::EqualZ2k<ValueType, K>(mValue, other.mValue);
  };

  /**
   * @brief Return a string representation of this element.
   */
  std::string ToString() const {
    return details::ToStringZ2k<ValueType, BitSize()>(mValue);
  };

  /**
   * @brief Write this element to a buffer.
   */
  void Write(unsigned char* dest) const {
    details::WriteZ2k<ValueType, BitSize()>(mValue, dest);
  };

 private:
  ValueType mValue;
};

}  // namespace scl

#endif  // SCL_MATH_Z2K_H
