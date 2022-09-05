/**
 * @file ops_gmp_ff.h
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

#ifndef SCL_MATH_OPS_GMP_FF_H
#define SCL_MATH_OPS_GMP_FF_H

#include <gmp.h>

#include <cmath>
#include <cstddef>
#include <sstream>
#include <string>

#include "scl/math/str.h"

namespace scl {
namespace details {

#define BITS_PER_LIMB static_cast<std::size_t>(mp_bits_per_limb)
#define BYTES_PER_LIMB sizeof(mp_limb_t)

#define SCL_COPY(out, in, size)              \
  do {                                       \
    for (std::size_t i = 0; i < size; ++i) { \
      *((out) + i) = *((in) + i);            \
    }                                        \
  } while (0)

/**
 * @brief Convert a value into montgomery form mod some prime.
 * @param value the value to convert
 * @param mod the prime
 * @tparam N the size of \value and \p mod in limbs.
 */
template <std::size_t N>
void ToMonty(mp_limb_t* value, const mp_limb_t* mod) {
  mp_limb_t qp[N + 1];
  mp_limb_t shift[2 * N] = {0};
  // multiply val by 2^{256}
  SCL_COPY(shift + N, value, N);
  // compute (val * 2^{256}) mod p
  mpn_tdiv_qr(qp, value, 0, shift, 2 * N, mod, N);
}

/**
 * @brief Perform a montgomery reduction.
 * @param val the value to reduce
 * @param mod the modulus
 * @param np a number n such that <code>2^{N} * a + mod * n == 1</code>
 * @tparam N the size of \p mod, \p np and half of \p val in limbs.
 */
template <std::size_t N>
void Redc(mp_limb_t* val, const mp_limb_t* mod, const mp_limb_t* np) {
  // https://cp-algorithms.com/algebra/montgomery_multiplication.html#montgomery-reduction
  mp_limb_t q[2 * N];
  // TODO: This multiplication can be optimized because we're only interested in
  // the result mod r = 2^{N}.
  mpn_mul_n(q, val, np, N);
  mp_limb_t c[2 * N];
  mpn_mul_n(c, q, mod, N);
  auto borrow = mpn_sub_n(c, val, c, 2 * N);

  SCL_COPY(val, c + N, N);

  if (borrow) {
    mpn_add_n(val, val, mod, N);
  }
}

/**
 * @brief Convert an integer into a multi-precision value.
 *
 * This function converts an integer into an \p N limb multi-precision integer
 * modulo \p mod. The function assumes that \p out has been zeroed.
 *
 * @param out result
 * @param value the int to convert from
 * @param mod a modulus
 */
template <std::size_t N>
void FromInt(mp_limb_t* out, const int value, const mp_limb_t* mod) {
  out[0] = std::abs(value);
  if (value < 0) {
    mpn_sub_n(out, mod, out, N);
  }
  ToMonty<N>(out, mod);
}

/**
 * @brief Perform a modular addition.
 * @param out the first operand and destination of result
 * @param op the second operand
 * @param mod the modulus
 */
template <std::size_t N>
void ModAdd(mp_limb_t* out, const mp_limb_t* op, const mp_limb_t* mod) {
  auto carry = mpn_add_n(out, out, op, N);
  if (carry || mpn_cmp(out, mod, N) >= 0) {
    mpn_sub_n(out, out, mod, N);
  }
}

/**
 * @brief Perform a modular subtraction.
 * @param out the first operand and destination of result
 * @param op the second operand
 * @param mod the modulus.
 */
template <std::size_t N>
void ModSub(mp_limb_t* out, const mp_limb_t* op, const mp_limb_t* mod) {
  auto carry = mpn_sub_n(out, out, op, N);
  if (carry) {
    mpn_add_n(out, out, mod, N);
  }
}

/**
 * @brief Perform a modular negation.
 * @param out the operand and destination of result
 * @param mod the modulus
 */
template <std::size_t N>
void ModNeg(mp_limb_t* out, const mp_limb_t* mod) {
  mpn_sub_n(out, mod, out, N);
}

/**
 * @brief Perform a modular multiplication in montgomery representation
 * @param out the first operand and destination of result
 * @param op the second operand
 * @param mod the modulus
 * @param np a constant used for montgomery reduction
 * @see Redc
 */
template <std::size_t N>
void ModMul(mp_limb_t* out, const mp_limb_t* op, const mp_limb_t* mod,
            const mp_limb_t* np) {
  mp_limb_t res[2 * N];
  mpn_mul_n(res, out, op, N);
  Redc<N>(res, mod, np);
  SCL_COPY(out, res, N);
}

/**
 * @brief Perform a modular squaring in montgomery representation
 * @param out the output
 * @param op the operand to square
 * @param mod the modulus
 * @param np a constant used for montgomery reduction
 */
template <std::size_t N>
void ModSqr(mp_limb_t* out, const mp_limb_t* op, const mp_limb_t* mod,
            const mp_limb_t* np) {
  mp_limb_t res[2 * N];
  mpn_sqr(res, op, N);
  Redc<N>(res, mod, np);
  SCL_COPY(out, res, N);
}

/**
 * @brief Checks if a bit has been set.
 */
inline bool TestBit(const mp_limb_t* v, std::size_t pos) {
  auto limb = pos / BITS_PER_LIMB;
  auto bit = pos % BITS_PER_LIMB;
  return ((v[limb] >> bit) & 1) == 1;
}

/**
 * @brief Modular exponentation
 *
 * This function performs a modular exponentation of a multiprecision integer in
 * montgomery form.
 *
 * @param out output. Must initially be equal to 1 in montgomery form
 * @param x the base
 * @param e the exponent
 * @param mod the modulus
 * @param np a constant used for montgomery reduction
 */
template <std::size_t N>
void ModExp(mp_limb_t* out, const mp_limb_t* x, const mp_limb_t* e,
            const mp_limb_t* mod, const mp_limb_t* np) {
  auto n = mpn_scan1(e, N * BITS_PER_LIMB);
  for (int i = n - 1; i >= 0; --i) {
    ModSqr<N>(out, out, mod, np);
    if (TestBit(e, i)) {
      ModMul<N>(out, x, mod, np);
    }
  }
}

template <std::size_t N>
void ModInvFermat(mp_limb_t* out, const mp_limb_t* op, const mp_limb_t* mod,
                  const mp_limb_t* mod_minus_2, const mp_limb_t* np) {
  if (mpn_zero_p(op, N)) {
    throw std::invalid_argument("0 not invertible modulo prime");
  }

  ModExp<N>(out, op, mod_minus_2, mod, np);
}

/**
 * @brief Compute a comparison between two values
 * @return a value x such that <code>R(x, 0) <==> R(lhs, rhs)</code>.
 */
template <std::size_t N>
int CompareValues(const mp_limb_t* lhs, const mp_limb_t* rhs) {
  return mpn_cmp(lhs, rhs, N);
}

void ReadLimb(mp_limb_t& lmb, const unsigned char* bytes,
              std::size_t bits_per_limb);

/**
 * @brief Read a value from a byte array.
 */
template <std::size_t N>
void ValueFromBytes(mp_limb_t* out, const unsigned char* src) {
  for (std::size_t i = 0; i < N; ++i) {
    ReadLimb(out[i], src + i * BYTES_PER_LIMB, BITS_PER_LIMB);
  }
}

std::size_t FindFirstNonZero(const std::string& s);

/**
 * @brief Print a value.
 */
template <std::size_t N>
std::string ToString(const mp_limb_t* val, const mp_limb_t* mod,
                     const mp_limb_t* np) {
  mp_limb_t padded[2 * N] = {0};
  SCL_COPY(padded, val, N);
  Redc<N>(padded, mod, np);

  static const char* kHexChars = "0123456789abcdef";
  std::stringstream ss;
  for (int i = N - 1; i >= 0; --i) {
    const auto v = padded[i];
    for (int j = BYTES_PER_LIMB - 1; j >= 0; --j) {
      const auto vv = v >> (j * 8);
      ss << kHexChars[(vv & 0xF0) >> 4];
      ss << kHexChars[vv & 0x0F];
    }
  }
  auto s = ss.str();
  // trim leading 0s
  auto n = FindFirstNonZero(s);
  if (n > 0) {
    s = s.substr(n, s.length() - 1);
  }
  if (s.length()) {
    return s;
  } else {
    return "0";
  }
}

template <std::size_t N>
void FromString(mp_limb_t* out, const mp_limb_t* mod, const std::string& str) {
  if (str.length()) {
    auto n = str.length();
    if (n > 64) {
      throw std::invalid_argument("hex string too large to parse");
    }

    std::string s = str;
    if (n % 2) {
      s = "0" + s;
      n++;
    }

    const auto m = 2 * BYTES_PER_LIMB;
    int c = (n - 1) / m;
    auto beg = s.begin();
    for (std::size_t i = 0; i < n && c >= 0; i += m) {
      auto end = std::min(n, i + m);
      out[c--] = FromHexString<mp_limb_t>(std::string(beg + i, beg + end));
    }
    ToMonty<N>(out, mod);
  }
}

#undef SCL_COPY
#undef BITS_PER_LIMB
#undef BYTES_PER_LIMB

}  // namespace details
}  // namespace scl

#endif  // SCL_MATH_OPS_GMP_FF_H
