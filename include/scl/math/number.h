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

#ifndef SCL_MATH_NUMBER_H
#define SCL_MATH_NUMBER_H

#include <cstdint>
#include <memory>

#include <gmp.h>

#include "scl/util/prg.h"

namespace scl::math {

/**
 * @brief Arbitrary precision integer.
 */
class Number {
 public:
  /**
   * @brief Generate a random Number.
   * @param bits the number of bits in the resulting number
   * @param prg a prg for generating the random number
   * @return a random Number
   */
  static Number Random(std::size_t bits, util::PRG& prg);

  /**
   * @brief Read a Number from a string
   * @param str the string
   * @return a Number.
   */
  static Number FromString(const std::string& str);

  /**
   * @brief Construct a Number from an int.
   * @param value the int
   */
  explicit Number(int value);

  /**
   * @brief Construct a Number with a default value of 0.
   */
  explicit Number();

  /**
   * @brief Clean up this number.
   */
  ~Number();

  /**
   * @brief Copy constructor for a Number.
   * @param number the Number that is copied
   */
  Number(const Number& number);

  /**
   * @brief Move constructor for a Number.
   * @param number the Number that is moved
   */
  Number(Number&& number) noexcept;

  /**
   * @brief Copy assignment from a Number.
   * @param number the Number that is copied
   * @return this
   */
  Number& operator=(const Number& number) {
    Number copy(number);
    swap(*this, copy);
    return *this;
  };

  /**
   * @brief Move assignment from a Number.
   * @param number the Number that is moved
   * @return this
   */
  Number& operator=(Number&& number) noexcept {
    swap(*this, number);
    return *this;
  };

  /**
   * @brief In-place addition of two numbers.
   * @param number the other number
   * @return this
   */
  Number& operator+=(const Number& number) {
    *this = *this + number;
    return *this;
  };

  /**
   * @brief Add two numbers.
   * @param number the other number
   * @return the sum of \p this and \p number.
   */
  Number operator+(const Number& number) const;

  /**
   * @brief In-place subtraction of two numbers.
   * @param number the other number
   * @return this.
   */
  Number& operator-=(const Number& number) {
    *this = *this - number;
    return *this;
  };

  /**
   * @brief Subtract two Numbers.
   * @param number the other number
   * @return the difference between \p this and \p number.
   */
  Number operator-(const Number& number) const;

  /**
   * @brief Negate this Number.
   * @return this, negated.
   */
  Number operator-() const;

  /**
   * @brief In-place multiplication of two numbers.
   * @param number the other Number
   * @return this.
   */
  Number& operator*=(const Number& number) {
    *this = *this * number;
    return *this;
  };

  /**
   * @brief Multiply two Numbers.
   * @param number the other number
   * @return the product of \p this and \p number.
   */
  Number operator*(const Number& number) const;

  /**
   * @brief In-place integer division of two Numbers.
   * @param number the other number
   * @return this.
   */
  Number& operator/=(const Number& number) {
    *this = *this / number;
    return *this;
  };

  /**
   * @brief Divide two Numbers.
   * @param number the other number
   * @return the division of \p this and \p number.
   */
  Number operator/(const Number& number) const;

  /**
   * @brief In-place left shift.
   * @param shift the amount to left shift
   * @return this.
   */
  Number& operator<<=(int shift) {
    *this = *this << shift;
    return *this;
  };

  /**
   * @brief Perform a left shift of a Number.
   * @param shift the amount to left shift
   * @return the number, left-shifted by \p shift bits.
   */
  Number operator<<(int shift) const;

  /**
   * @brief In-place right shift
   * @param shift the amount to right shift
   * @return this.
   */
  Number& operator>>=(int shift) {
    *this = *this >> shift;
    return *this;
  };

  /**
   * @brief Perform a right shift of a Number.
   * @param shift the amount to right shift
   * @return the number, right-shifted by \p shift bits.
   */
  Number operator>>(int shift) const;

  /**
   * @brief In-place exclusive or.
   * @param number the number to xor this with
   * @return \p this
   */
  Number& operator^=(const Number& number) {
    *this = *this ^ number;
    return *this;
  };

  /**
   * @brief Exclusive or of two numbers.
   * @param number the other number
   * @return the xor of \p this and \p number.
   */
  Number operator^(const Number& number) const;

  /**
   * @brief operator |=
   * @param number
   * @return
   */
  Number& operator|=(const Number& number) {
    *this = *this | number;
    return *this;
  };

  /**
   * @brief operator |
   * @param number
   * @return
   */
  Number operator|(const Number& number) const;

  /**
   * @brief operator &=
   * @param number
   * @return
   */
  Number& operator&=(const Number& number) {
    *this = *this & number;
    return *this;
  };

  /**
   * @brief operator &
   * @param number
   * @return
   */
  Number operator&(const Number& number) const;

  /**
   * @brief operator ~
   * @return
   */
  Number operator~() const;

  /**
   * @brief Perform a comparison of two numbers.
   *
   * This method computes an integer \f$c\f$ between this number and another
   * number such that \f$R(c,0) \equiv R(a,b)\f$ where \f$a,b\f$ are \p this and
   * \p number, respectively. For example, if the return value is equal to 0,
   * then \p this and \p number are identical.
   *
   * @param number the other number
   * @return a int indicating the relationship between this and \p number.
   */
  int Compare(const Number& number) const;

  /**
   * @brief Equality of two numbers.
   */
  friend bool operator==(const Number& lhs, const Number& rhs) {
    return lhs.Compare(rhs) == 0;
  };

  /**
   * @brief In-equality of two numbers.
   */
  friend bool operator!=(const Number& lhs, const Number& rhs) {
    return lhs.Compare(rhs) != 0;
  };

  /**
   * @brief Strictly less-than of two numbers.
   */
  friend bool operator<(const Number& lhs, const Number& rhs) {
    return lhs.Compare(rhs) < 0;
  };

  /**
   * @brief Less-than-or-equal of two numbers.
   */
  friend bool operator<=(const Number& lhs, const Number& rhs) {
    return lhs.Compare(rhs) <= 0;
  };

  /**
   * @brief Strictly greater-than of two numbers.
   */
  friend bool operator>(const Number& lhs, const Number& rhs) {
    return lhs.Compare(rhs) > 0;
  };

  /**
   * @brief Greater-than-or-equal of two numbers.
   */
  friend bool operator>=(const Number& lhs, const Number& rhs) {
    return lhs.Compare(rhs) >= 0;
  };

  /**
   * @brief Get the size of this Number in bits.
   */
  std::size_t BitSize() const;

  /**
   * @brief Test whether a particular bit of this Number is set.
   *
   * This method will test the bit similar to a shift by \p index. In
   * particular, false is returned if \p index is larger than the bit-size of
   * this Number.
   *
   * @param index the index of the bit
   * @return true if the bit at \p index is set and false otherwise.
   */
  bool TestBit(std::size_t index) const;

  /**
   * @brief Test if this Number is odd.
   * @return true if this Number is odd.
   */
  bool Odd() const {
    return TestBit(0);
  };

  /**
   * @brief Test if this Number is even.
   * @return true if this Number is even.
   */
  bool Even() const {
    return !Odd();
  };

  /**
   * @brief Return a string representation of this Number.
   * @return a string.
   */
  std::string ToString() const;

  /**
   * @brief << overload for STL ostream objects.
   */
  friend std::ostream& operator<<(std::ostream& os, const Number& number) {
    return os << number.ToString();
  };

  /**
   * @brief STL swap implementation for Number.
   */
  friend void swap(Number& first, Number& second) {
    using std::swap;
    swap(first.m_value, second.m_value);
  };

 private:
  mpz_t m_value;
};

}  // namespace scl::math

#endif  // SCL_MATH_NUMBER_H
