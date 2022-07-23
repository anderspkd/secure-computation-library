/**
 * @file number.cc
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

#include "scl/math/number.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <type_traits>

scl::Number::Number() { mpz_init(mValue); }

scl::Number::Number(const Number& number) : Number() {
  mpz_set(mValue, number.mValue);
}

scl::Number::Number(Number&& number) : Number() {
  mpz_set(mValue, number.mValue);
}

scl::Number::~Number() { mpz_clear(mValue); }

scl::Number scl::Number::Random(std::size_t bits, PRG& prg) {
  auto len = (bits - 1) / 8 + 2;
  auto data = std::make_unique<unsigned char[]>(len);
  prg.Next(data.get(), len);

  // trim trailing bits to ensure the resulting number is atmost bits large
  data[1] &= (1 << (bits % 8)) - 1;

  scl::Number r;
  mpz_import(r.mValue, len - 1, 1, 1, 0, 0, data.get() + 1);
  if (data[0] & 1) {
    mpz_neg(r.mValue, r.mValue);
  }
  return r;
}

scl::Number scl::Number::FromString(const std::string& str) {
  scl::Number num;
  mpz_set_str(num.mValue, str.c_str(), 16);
  return num;
}  // LCOV_EXCL_LINE

scl::Number::Number(int value) : Number() { mpz_set_si(mValue, value); }

scl::Number scl::Number::operator+(const Number& number) const {
  scl::Number sum;
  mpz_add(sum.mValue, mValue, number.mValue);
  return sum;
}  // LCOV_EXCL_LINE

scl::Number scl::Number::operator-(const Number& number) const {
  scl::Number diff;
  mpz_sub(diff.mValue, mValue, number.mValue);
  return diff;
}  // LCOV_EXCL_LINE

scl::Number scl::Number::operator-() const {
  scl::Number neg;
  mpz_neg(neg.mValue, mValue);
  return neg;
}  // LCOV_EXCL_LINE

scl::Number scl::Number::operator*(const Number& number) const {
  scl::Number prod;
  mpz_mul(prod.mValue, mValue, number.mValue);
  return prod;
}  // LCOV_EXCL_LINE

scl::Number scl::Number::operator/(const Number& number) const {
  scl::Number frac;
  mpz_div(frac.mValue, mValue, number.mValue);
  return frac;
}  // LCOV_EXCL_LINE

scl::Number scl::Number::operator<<(int shift) const {
  if (shift < 0) {
    return operator>>(-shift);
  } else {
    scl::Number shifted;
    mpz_mul_2exp(shifted.mValue, mValue, shift);
    return shifted;
  }
}

scl::Number scl::Number::operator>>(int shift) const {
  if (shift < 0) {
    return operator<<(-shift);
  } else {
    scl::Number shifted;
    mpz_tdiv_q_2exp(shifted.mValue, mValue, shift);
    return shifted;
  }
}

scl::Number scl::Number::operator^(const Number& number) const {
  scl::Number xord;
  mpz_xor(xord.mValue, mValue, number.mValue);
  return xord;
}  // LCOV_EXCL_LINE

scl::Number scl::Number::operator|(const Number& number) const {
  scl::Number ord;
  mpz_ior(ord.mValue, mValue, number.mValue);
  return ord;
}  // LCOV_EXCL_LINE

scl::Number scl::Number::operator&(const Number& number) const {
  scl::Number andd;
  mpz_and(andd.mValue, mValue, number.mValue);
  return andd;
}  // LCOV_EXCL_LINE

scl::Number scl::Number::operator~() const {
  scl::Number com;
  mpz_com(com.mValue, mValue);
  return com;
}  // LCOV_EXCL_LINE

int scl::Number::Compare(const Number& number) const {
  return mpz_cmp(mValue, number.mValue);
}

std::size_t scl::Number::BitSize() const { return mpz_sizeinbase(mValue, 2); }

bool scl::Number::TestBit(std::size_t index) const {
  return mpz_tstbit(mValue, index);
}

std::string scl::Number::ToString() const {
  char* cstr;
  cstr = mpz_get_str(nullptr, 16, mValue);
  std::stringstream ss;
  ss << "Number{" << std::string(cstr) << "}";
  free(cstr);
  return ss.str();
}
