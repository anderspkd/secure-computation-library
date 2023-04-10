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

#include "scl/math/number.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <type_traits>

scl::math::Number::Number() {
  mpz_init(m_value);
}

scl::math::Number::Number(const Number& number) : Number() {
  mpz_set(m_value, number.m_value);
}

scl::math::Number::Number(Number&& number) noexcept : Number() {
  mpz_set(m_value, number.m_value);
}

scl::math::Number::~Number() {
  mpz_clear(m_value);
}

scl::math::Number scl::math::Number::Random(std::size_t bits, util::PRG& prg) {
  auto len = (bits - 1) / 8 + 2;
  auto data = std::make_unique<unsigned char[]>(len);
  prg.Next(data.get(), len);

  // trim trailing bits to ensure the resulting number is atmost bits large
  data[1] &= (1 << (bits % 8)) - 1;

  scl::math::Number r;
  mpz_import(r.m_value, len - 1, 1, 1, 0, 0, data.get() + 1);
  if ((data[0] & 1) != 0) {
    mpz_neg(r.m_value, r.m_value);
  }
  return r;
}

scl::math::Number scl::math::Number::FromString(const std::string& str) {
  scl::math::Number num;
  mpz_set_str(num.m_value, str.c_str(), 16);
  return num;
}  // LCOV_EXCL_LINE

scl::math::Number::Number(int value) : Number() {
  mpz_set_si(m_value, value);
}

scl::math::Number scl::math::Number::operator+(const Number& number) const {
  scl::math::Number sum;
  mpz_add(sum.m_value, m_value, number.m_value);
  return sum;
}  // LCOV_EXCL_LINE

scl::math::Number scl::math::Number::operator-(const Number& number) const {
  scl::math::Number diff;
  mpz_sub(diff.m_value, m_value, number.m_value);
  return diff;
}  // LCOV_EXCL_LINE

scl::math::Number scl::math::Number::operator-() const {
  scl::math::Number neg;
  mpz_neg(neg.m_value, m_value);
  return neg;
}  // LCOV_EXCL_LINE

scl::math::Number scl::math::Number::operator*(const Number& number) const {
  scl::math::Number prod;
  mpz_mul(prod.m_value, m_value, number.m_value);
  return prod;
}  // LCOV_EXCL_LINE

scl::math::Number scl::math::Number::operator/(const Number& number) const {
  if (mpz_sgn(number.m_value) == 0) {
    throw std::logic_error("division by 0");
  }
  scl::math::Number frac;
  mpz_div(frac.m_value, m_value, number.m_value);
  return frac;
}  // LCOV_EXCL_LINE

scl::math::Number scl::math::Number::operator<<(int shift) const {
  scl::math::Number shifted;
  if (shift < 0) {
    shifted = operator>>(-shift);
  } else {
    mpz_mul_2exp(shifted.m_value, m_value, shift);
  }
  return shifted;
}  // LCOV_EXCL_LINE

scl::math::Number scl::math::Number::operator>>(int shift) const {
  scl::math::Number shifted;
  if (shift < 0) {
    shifted = operator<<(-shift);
  } else {
    mpz_tdiv_q_2exp(shifted.m_value, m_value, shift);
  }
  return shifted;
}  // LCOV_EXCL_LINE

scl::math::Number scl::math::Number::operator^(const Number& number) const {
  scl::math::Number xord;
  mpz_xor(xord.m_value, m_value, number.m_value);
  return xord;
}  // LCOV_EXCL_LINE

scl::math::Number scl::math::Number::operator|(const Number& number) const {
  scl::math::Number ord;
  mpz_ior(ord.m_value, m_value, number.m_value);
  return ord;
}  // LCOV_EXCL_LINE

scl::math::Number scl::math::Number::operator&(const Number& number) const {
  scl::math::Number andd;
  mpz_and(andd.m_value, m_value, number.m_value);
  return andd;
}  // LCOV_EXCL_LINE

scl::math::Number scl::math::Number::operator~() const {
  scl::math::Number com;
  mpz_com(com.m_value, m_value);
  return com;
}  // LCOV_EXCL_LINE

int scl::math::Number::Compare(const Number& number) const {
  return mpz_cmp(m_value, number.m_value);
}

std::size_t scl::math::Number::BitSize() const {
  return mpz_sizeinbase(m_value, 2);
}

bool scl::math::Number::TestBit(std::size_t index) const {
  return mpz_tstbit(m_value, index);
}

std::string scl::math::Number::ToString() const {
  char* cstr;
  cstr = mpz_get_str(nullptr, 16, m_value);
  std::stringstream ss;
  ss << "Number{" << std::string(cstr) << "}";
  free(cstr);
  return ss.str();
}
