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

#ifndef SCL_MATH_OPS_GMP_FF_H
#define SCL_MATH_OPS_GMP_FF_H

#include <cmath>
#include <cstddef>
#include <cstring>
#include <sstream>
#include <string>

#include <gmp.h>

#include "scl/util/str.h"

namespace scl::math {

#define SCL_BITS_PER_LIMB static_cast<std::size_t>(mp_bits_per_limb)
#define SCL_BYTES_PER_LIMB sizeof(mp_limb_t)

#define SCL_COPY(out, in, size)                \
  do {                                         \
    for (std::size_t i = 0; i < (size); ++i) { \
      *((out) + i) = *((in) + i);              \
    }                                          \
  } while (0)

/**
 * @brief Convert a value into montgomery form mod some prime.
 * @tparam N the size of the input
 * @param value the value to convert
 * @param mod the prime
 */
template <std::size_t N>
void MontyIn(mp_limb_t* value, const mp_limb_t* mod) {
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
 * @tparam N the size of the input
 */
template <std::size_t N>
void MontyRedc(mp_limb_t* val, const mp_limb_t* mod, const mp_limb_t* np) {
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
 * @param out result
 * @param value the int to convert from
 * @param mod a modulus
 *
 * This function converts an integer into an \p N limb multi-precision integer
 * modulo \p mod. The function assumes that \p out has been zeroed.
 */
template <std::size_t N>
void MontyInFromInt(mp_limb_t* out, const int value, const mp_limb_t* mod) {
  out[0] = std::abs(value);
  if (value < 0) {
    mpn_sub_n(out, mod, out, N);
  }
  MontyIn<N>(out, mod);
}

/**
 * @brief Perform a modular addition.
 * @param out the first operand and destination of result
 * @param op the second operand
 * @param mod the modulus
 */
template <std::size_t N>
void MontyModAdd(mp_limb_t* out, const mp_limb_t* op, const mp_limb_t* mod) {
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
void MontyModSub(mp_limb_t* out, const mp_limb_t* op, const mp_limb_t* mod) {
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
void MontyModNeg(mp_limb_t* out, const mp_limb_t* mod) {
  mp_limb_t t[N] = {0};
  MontyModSub<N>(t, out, mod);
  SCL_COPY(out, t, N);
}

/**
 * @brief Perform a modular multiplication in montgomery representation
 * @param out the first operand and destination of result
 * @param op the second operand
 * @param mod the modulus
 * @param np a constant used for montgomery reduction
 * @see MontyRedc
 */
template <std::size_t N>
void MontyModMul(mp_limb_t* out,
                 const mp_limb_t* op,
                 const mp_limb_t* mod,
                 const mp_limb_t* np) {
  mp_limb_t res[2 * N];
  mpn_mul_n(res, out, op, N);
  MontyRedc<N>(res, mod, np);
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
void MontyModSqr(mp_limb_t* out,
                 const mp_limb_t* op,
                 const mp_limb_t* mod,
                 const mp_limb_t* np) {
  mp_limb_t res[2 * N];
  mpn_sqr(res, op, N);
  MontyRedc<N>(res, mod, np);
  SCL_COPY(out, res, N);
}

/**
 * @brief Checks if a bit has been set.
 */
inline bool TestBit(const mp_limb_t* v, std::size_t pos) {
  auto limb = pos / SCL_BITS_PER_LIMB;
  auto bit = pos % SCL_BITS_PER_LIMB;
  return ((v[limb] >> bit) & 1) == 1;
}

/**
 * @brief Modular exponentation
 * @param out output. Must initially be equal to 1 in montgomery form
 * @param x the base
 * @param e the exponent
 * @param mod the modulus
 * @param np a constant used for montgomery reduction
 *
 * This function performs a modular exponentation of a multiprecision integer in
 * montgomery form.
 */
template <std::size_t N>
void MontyModExp(mp_limb_t* out,
                 const mp_limb_t* x,
                 const mp_limb_t* e,
                 const mp_limb_t* mod,
                 const mp_limb_t* np) {
  auto n = mpn_sizeinbase(e, N, 2);
  for (std::size_t i = n; i-- > 0;) {
    MontyModSqr<N>(out, out, mod, np);
    if (TestBit(e, i)) {
      MontyModMul<N>(out, x, mod, np);
    }
  }
}

/**
 * @brief Compute a modular inverse.
 * @param out output destination
 * @param op the value to invert
 * @param mod the modulus
 * @param mod_minus_2 \p mod minus 2
 * @param np a constant used for montgomery reduction
 */
template <std::size_t N>
void MontyModInv(mp_limb_t* out,
                 const mp_limb_t* op,
                 const mp_limb_t* mod,
                 const mp_limb_t* mod_minus_2,
                 const mp_limb_t* np) {
  if (mpn_zero_p(op, N)) {
    throw std::invalid_argument("0 not invertible modulo prime");
  }

  MontyModExp<N>(out, op, mod_minus_2, mod, np);
}

/**
 * @brief Compute a comparison between two values
 * @return a value x such that <code>R(x, 0) <==> R(lhs, rhs)</code>.
 */
template <std::size_t N>
int CompareValues(const mp_limb_t* lhs, const mp_limb_t* rhs) {
  return mpn_cmp(lhs, rhs, N);
}

/**
 * @brief Deserialize a value and convert to montgomery form.
 * @param out output destination
 * @param src where to read the value from
 * @param mod the modulus
 */
template <std::size_t N>
void MontyFromBytes(mp_limb_t* out,
                    const unsigned char* src,
                    const mp_limb_t* mod) {
  for (int i = N - 1; i >= 0; --i) {
    for (int j = SCL_BYTES_PER_LIMB - 1; j >= 0; --j) {
      out[i] |= static_cast<mp_limb_t>(*src++) << (j * 8);
    }
  }

  MontyIn<N>(out, mod);
}

/**
 * @brief Write a value in montgomery form to a buffer.
 * @param dest the output buffer
 * @param src the input value
 * @param mod the modulus
 * @param np a montgomery constant
 */
template <std::size_t N>
void MontyToBytes(unsigned char* dest,
                  const mp_limb_t* src,
                  const mp_limb_t* mod,
                  const mp_limb_t* np) {
  mp_limb_t padded[2 * N] = {0};
  SCL_COPY(padded, src, N);
  MontyRedc<N>(padded, mod, np);

  std::size_t c = 0;
  for (int i = N - 1; i >= 0; --i) {
    const auto v = padded[i];
    for (int j = SCL_BYTES_PER_LIMB - 1; j >= 0; --j) {
      dest[c++] = v >> (j * 8);
    }
  }
}

/**
 * @brief Find the first non-zero character in a string.
 *
 * This method is used handle a string representation of a number with leading
 * zeros.
 */
std::size_t FindFirstNonZero(const std::string& s);

/**
 * @brief Print a value.
 */
template <std::size_t N>
std::string MontyToString(const mp_limb_t* val,
                          const mp_limb_t* mod,
                          const mp_limb_t* np) {
  mp_limb_t padded[2 * N] = {0};
  SCL_COPY(padded, val, N);
  MontyRedc<N>(padded, mod, np);

  static const char* kHexChars = "0123456789abcdef";
  std::stringstream ss;
  for (int i = N - 1; i >= 0; --i) {
    const auto v = padded[i];
    for (int j = SCL_BYTES_PER_LIMB - 1; j >= 0; --j) {
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
  }

  return "0";
}

/**
 * @brief Read a value from a string.
 */
template <std::size_t N>
void MontyFromString(mp_limb_t* out,
                     const mp_limb_t* mod,
                     const std::string& str) {
  if (str.length()) {
    auto n_ = str.length();
    if (n_ > 64) {
      throw std::invalid_argument("hex string too large to parse");
    }
    // to silence conversion errors. Safe to do because n_ is pretty small.
    int n = (int)n_;

    std::string s = str;
    if (n % 2) {
      s = "0" + s;
      n++;
    }

    const auto m = static_cast<int>(2 * SCL_BYTES_PER_LIMB);
    int c = (n - 1) / m;
    auto beg = s.begin();
    for (int i = 0; i < n && c >= 0; i += m) {
      auto end = std::min(n, i + m);
      out[c--] =
          util::FromHexString<mp_limb_t>(std::string(beg + i, beg + end));
    }
    MontyIn<N>(out, mod);
  }
}

#undef SCL_BITS_PER_LIMB
#undef SCL_BYTES_PER_LIMB
#undef SCL_COPY

}  // namespace scl::math

#endif  // SCL_MATH_OPS_GMP_FF_H
