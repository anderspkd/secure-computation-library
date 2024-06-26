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

#include "scl/math/number.h"

#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <type_traits>

#include <gmp.h>

using namespace scl;

math::Number::Number() {
  mpz_init(m_value);
}

math::Number::Number(const Number& number) : Number() {
  mpz_set(m_value, number.m_value);
}

math::Number::Number(Number&& number) noexcept : Number() {
  mpz_set(m_value, number.m_value);
}

math::Number::~Number() {
  mpz_clear(m_value);
}

math::Number math::Number::random(std::size_t bits, util::PRG& prg) {
  auto len = (bits - 1) / 8 + 2;
  auto data = std::make_unique<unsigned char[]>(len);
  prg.next(data.get(), len);

  // trim trailing bits to ensure the resulting number is atmost bits large
  data[1] &= (1 << (bits % 8)) - 1;

  math::Number r;
  mpz_import(r.m_value, len - 1, 1, 1, 0, 0, data.get() + 1);
  if ((data[0] & 1) != 0) {
    mpz_neg(r.m_value, r.m_value);
  }
  return r;
}

math::Number math::Number::randomPrime(std::size_t bits, util::PRG& prg) {
  auto r = random(bits, prg);
  Number prime;
  mpz_nextprime(prime.m_value, r.m_value);
  return prime;
}

math::Number math::Number::fromString(const std::string& str) {
  math::Number num;
  mpz_set_str(num.m_value, str.c_str(), 16);
  return num;
}  // LCOV_EXCL_LINE

math::Number math::Number::read(const unsigned char* buf) {
  std::uint32_t size_and_sign;
  std::memcpy(&size_and_sign, buf, sizeof(std::uint32_t));

  bool negative = (size_and_sign >> 31) == 1;
  auto size = size_and_sign & ((1 << 30) - 1);

  Number r;
  mpz_import(r.m_value, size, 1, 1, 0, 0, buf + sizeof(std::uint32_t));
  if (negative) {
    mpz_neg(r.m_value, r.m_value);
  }
  return r;
}  // LCOV_EXCL_LINE

math::Number::Number(int value) : Number() {
  mpz_set_si(m_value, value);
}

math::Number math::Number::operator+(const Number& number) const {
  math::Number sum;
  mpz_add(sum.m_value, m_value, number.m_value);
  return sum;
}  // LCOV_EXCL_LINE

math::Number math::Number::operator-(const Number& number) const {
  math::Number diff;
  mpz_sub(diff.m_value, m_value, number.m_value);
  return diff;
}  // LCOV_EXCL_LINE

math::Number math::Number::operator-() const {
  math::Number neg;
  mpz_neg(neg.m_value, m_value);
  return neg;
}  // LCOV_EXCL_LINE

math::Number math::Number::operator*(const Number& number) const {
  math::Number prod;
  mpz_mul(prod.m_value, m_value, number.m_value);
  return prod;
}  // LCOV_EXCL_LINE

math::Number math::Number::operator/(const Number& number) const {
  if (mpz_sgn(number.m_value) == 0) {
    throw std::logic_error("division by 0");
  }
  math::Number frac;
  mpz_div(frac.m_value, m_value, number.m_value);
  return frac;
}  // LCOV_EXCL_LINE

math::Number math::Number::operator%(const Number& mod) const {
  math::Number res;
  mpz_mod(res.m_value, m_value, mod.m_value);
  return res;
}  // LCOV_EXCL_LINE

math::Number math::Number::operator<<(int shift) const {
  math::Number shifted;
  if (shift < 0) {
    shifted = operator>>(-shift);
  } else {
    mpz_mul_2exp(shifted.m_value, m_value, shift);
  }
  return shifted;
}  // LCOV_EXCL_LINE

math::Number math::Number::operator>>(int shift) const {
  math::Number shifted;
  if (shift < 0) {
    shifted = operator<<(-shift);
  } else {
    mpz_tdiv_q_2exp(shifted.m_value, m_value, shift);
  }
  return shifted;
}  // LCOV_EXCL_LINE

math::Number math::Number::operator^(const Number& number) const {
  math::Number xord;
  mpz_xor(xord.m_value, m_value, number.m_value);
  return xord;
}  // LCOV_EXCL_LINE

math::Number math::Number::operator|(const Number& number) const {
  math::Number ord;
  mpz_ior(ord.m_value, m_value, number.m_value);
  return ord;
}  // LCOV_EXCL_LINE

math::Number math::Number::operator&(const Number& number) const {
  math::Number andd;
  mpz_and(andd.m_value, m_value, number.m_value);
  return andd;
}  // LCOV_EXCL_LINE

math::Number math::Number::operator~() const {
  math::Number com;
  mpz_com(com.m_value, m_value);
  return com;
}  // LCOV_EXCL_LINE

int math::Number::compare(const Number& number) const {
  return mpz_cmp(m_value, number.m_value);
}

std::size_t math::Number::byteSize() const {
  return (bitSize() - 1) / 8 + 1;
}

std::size_t math::Number::bitSize() const {
  return mpz_sizeinbase(m_value, 2);
}

bool math::Number::testBit(std::size_t index) const {
  return mpz_tstbit(m_value, index);
}

std::string math::Number::toString() const {
  char* cstr;
  cstr = mpz_get_str(nullptr, 16, m_value);
  std::stringstream ss;
  ss << "Number{" << std::string(cstr) << "}";
  free(cstr);
  return ss.str();
}

void math::Number::write(unsigned char* buf) const {
  std::uint32_t size_and_sign = byteSize();

  if (mpz_sgn(m_value) < 0) {
    size_and_sign |= (1 << 31);
  }

  std::memcpy(buf, &size_and_sign, sizeof(std::uint32_t));
  mpz_export(buf + sizeof(std::uint32_t), NULL, 1, 1, 0, 0, m_value);
}

math::Number math::lcm(const Number& a, const Number& b) {
  Number lcm;
  mpz_lcm(lcm.m_value, a.m_value, b.m_value);
  return lcm;
}  // LCOV_EXCL_LINE

math::Number math::gcd(const Number& a, const Number& b) {
  Number gcd;
  mpz_gcd(gcd.m_value, a.m_value, b.m_value);
  return gcd;
}  // LCOV_EXCL_LINE

math::Number math::modInverse(const Number& val, const Number& mod) {
  if (mpz_sgn(mod.m_value) == 0) {
    throw std::invalid_argument("modulus cannot be 0");
  }

  Number inv;
  auto e = mpz_invert(inv.m_value, val.m_value, mod.m_value);
  if (e == 0) {
    throw std::logic_error("number not invertible");
  }

  return inv;
}  // LCOV_EXCL_LINE

math::Number math::modExp(const Number& base,
                          const Number& exp,
                          const Number& mod) {
  Number r;
  mpz_powm(r.m_value, base.m_value, exp.m_value, mod.m_value);
  return r;
}  // LCOV_EXCL_LINE
