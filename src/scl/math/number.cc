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
  mpz_init(mValue);
}

scl::math::Number::Number(const Number& number) : Number() {
  mpz_set(mValue, number.mValue);
}

scl::math::Number::Number(Number&& number) noexcept : Number() {
  mpz_set(mValue, number.mValue);
}

scl::math::Number::~Number() {
  mpz_clear(mValue);
}

scl::math::Number scl::math::Number::Random(std::size_t bits, util::PRG& prg) {
  auto len = (bits - 1) / 8 + 2;
  auto data = std::make_unique<unsigned char[]>(len);
  prg.Next(data.get(), len);

  // trim trailing bits to ensure the resulting number is atmost bits large
  data[1] &= (1 << (bits % 8)) - 1;

  scl::math::Number r;
  mpz_import(r.mValue, len - 1, 1, 1, 0, 0, data.get() + 1);
  if ((data[0] & 1) != 0) {
    mpz_neg(r.mValue, r.mValue);
  }
  return r;
}

scl::math::Number scl::math::Number::FromString(const std::string& str) {
  scl::math::Number num;
  mpz_set_str(num.mValue, str.c_str(), 16);
  return num;
}  // LCOV_EXCL_LINE

scl::math::Number::Number(int value) : Number() {
  mpz_set_si(mValue, value);
}

scl::math::Number scl::math::Number::operator+(const Number& number) const {
  scl::math::Number sum;
  mpz_add(sum.mValue, mValue, number.mValue);
  return sum;
}  // LCOV_EXCL_LINE

scl::math::Number scl::math::Number::operator-(const Number& number) const {
  scl::math::Number diff;
  mpz_sub(diff.mValue, mValue, number.mValue);
  return diff;
}  // LCOV_EXCL_LINE

scl::math::Number scl::math::Number::operator-() const {
  scl::math::Number neg;
  mpz_neg(neg.mValue, mValue);
  return neg;
}  // LCOV_EXCL_LINE

scl::math::Number scl::math::Number::operator*(const Number& number) const {
  scl::math::Number prod;
  mpz_mul(prod.mValue, mValue, number.mValue);
  return prod;
}  // LCOV_EXCL_LINE

scl::math::Number scl::math::Number::operator/(const Number& number) const {
  if (mpz_sgn(number.mValue) == 0) {
    throw std::logic_error("division by 0");
  }
  scl::math::Number frac;
  mpz_div(frac.mValue, mValue, number.mValue);
  return frac;
}  // LCOV_EXCL_LINE

scl::math::Number scl::math::Number::operator<<(int shift) const {
  scl::math::Number shifted;
  if (shift < 0) {
    shifted = operator>>(-shift);
  } else {
    mpz_mul_2exp(shifted.mValue, mValue, shift);
  }
  return shifted;
}  // LCOV_EXCL_LINE

scl::math::Number scl::math::Number::operator>>(int shift) const {
  scl::math::Number shifted;
  if (shift < 0) {
    shifted = operator<<(-shift);
  } else {
    mpz_tdiv_q_2exp(shifted.mValue, mValue, shift);
  }
  return shifted;
}  // LCOV_EXCL_LINE

scl::math::Number scl::math::Number::operator^(const Number& number) const {
  scl::math::Number xord;
  mpz_xor(xord.mValue, mValue, number.mValue);
  return xord;
}  // LCOV_EXCL_LINE

scl::math::Number scl::math::Number::operator|(const Number& number) const {
  scl::math::Number ord;
  mpz_ior(ord.mValue, mValue, number.mValue);
  return ord;
}  // LCOV_EXCL_LINE

scl::math::Number scl::math::Number::operator&(const Number& number) const {
  scl::math::Number andd;
  mpz_and(andd.mValue, mValue, number.mValue);
  return andd;
}  // LCOV_EXCL_LINE

scl::math::Number scl::math::Number::operator~() const {
  scl::math::Number com;
  mpz_com(com.mValue, mValue);
  return com;
}  // LCOV_EXCL_LINE

int scl::math::Number::Compare(const Number& number) const {
  return mpz_cmp(mValue, number.mValue);
}

std::size_t scl::math::Number::BitSize() const {
  return mpz_sizeinbase(mValue, 2);
}

bool scl::math::Number::TestBit(std::size_t index) const {
  return mpz_tstbit(mValue, index);
}

std::string scl::math::Number::ToString() const {
  char* cstr;
  cstr = mpz_get_str(nullptr, 16, mValue);
  std::stringstream ss;
  ss << "Number{" << std::string(cstr) << "}";
  free(cstr);
  return ss.str();
}
