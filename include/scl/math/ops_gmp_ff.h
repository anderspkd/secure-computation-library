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

#include <algorithm>
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

/**
 * @brief Reduction parameters used to perform Montgomery reduction.
 * @tparam N the number of words in the parameters.
 *
 * This struct is used to perform Montgomery modular reductions and is used
 * throughout all <code>Monty*</code> functions.
 */
template <std::size_t N>
struct RedParams {
  /**
   * @brief The prime.
   */
  mp_limb_t prime[N];

  /**
   * @brief A constant used in montgomery reduction.
   *
   * This constant is computed as \f$mc = -prime^{-1} \mod 2^{w * N}\f$ where
   * \f$w\f$ is the word size in bits (probably 64).
   */
  mp_limb_t mc[N];
};

/**
 * @brief Convert a value into montgomery form mod some prime.
 * @tparam N the number of limbs in the value to convert.
 * @param out the value to convert.
 * @param rp reduction parameters.
 */
template <std::size_t N>
void MontyIn(mp_limb_t* out, const RedParams<N> rp) {
  mp_limb_t qp[N + 1];
  mp_limb_t shift[2 * N] = {0};
  // multiply val by 2^{w * N}
  std::copy(out, out + N, shift + N);
  // compute (val * 2^{w * N}) mod p
  mpn_tdiv_qr(qp, out, 0, shift, 2 * N, rp.prime, N);
}

/**
 * @brief Perform a montgomery reduction.
 * @tparam N the number of limbs in the value to reduce.
 * @param out the value to reduce.
 * @param rp reduction parameters.
 */
template <std::size_t N>
void MontyRedc(mp_limb_t* out, const RedParams<N> rp) {
  // q = val * rp.mc
  // TODO: This can be optimized a bit since q is reduced modulo 2^N below
  mp_limb_t q[2 * N];
  mpn_mul_n(q, out, rp.mc, N);

  // c = (q mod 2^N) * rp.prime
  mp_limb_t c[2 * N];
  mpn_mul_n(c, q, rp.prime, N);

  // val + c / 2^256
  const auto carry = mpn_add_n(c, out, c, 2 * N);
  std::copy(c + N, c + 2 * N, out);

  if (carry || mpn_cmp(out, rp.prime, N) >= 0) {
    mpn_sub_n(out, out, rp.prime, N);
  }
}

/**
 * @brief Convert an integer into a value.
 * @tparam N the number of limbs in the output.
 * @param out destination of the converted value.
 * @param value the int to convert from.
 * @param rp reduction parameters.
 *
 * This function converts an integer into an \p N limb multi-precision integer
 * modulo \p mod. The function assumes that \p out has been zeroed.
 */
template <std::size_t N>
void MontyInFromInt(mp_limb_t* out, const int value, const RedParams<N> rp) {
  out[0] = std::abs(value);
  if (value < 0) {
    mpn_sub_n(out, rp.prime, out, N);
  }
  MontyIn<N>(out, rp);
}

/**
 * @brief Perform a modular addition.
 * @tparam N the number of limbs in the values to add.
 * @param out the first operand and destination of result.
 * @param op the second operand.
 * @param rp reduction parameters.
 */
template <std::size_t N>
void MontyModAdd(mp_limb_t* out, const mp_limb_t* op, const RedParams<N> rp) {
  auto carry = mpn_add_n(out, out, op, N);
  if (carry || mpn_cmp(out, rp.prime, N) >= 0) {
    mpn_sub_n(out, out, rp.prime, N);
  }
}

/**
 * @brief Perform a modular subtraction.
 * @tparam N the number of limbs in the values to subtract.
 * @param out the first operand and destination of result.
 * @param op the second operand.
 * @param rp reduction parameters.
 */
template <std::size_t N>
void MontyModSub(mp_limb_t* out, const mp_limb_t* op, const RedParams<N> rp) {
  auto carry = mpn_sub_n(out, out, op, N);
  if (carry) {
    mpn_add_n(out, out, rp.prime, N);
  }
}

/**
 * @brief Perform a modular negation.
 * @tparam N the number of limbs in the value to negate.
 * @param out the operand and destination of result.
 * @param rp reduction parameters.
 */
template <std::size_t N>
void MontyModNeg(mp_limb_t* out, const RedParams<N> rp) {
  mp_limb_t t[N] = {0};
  MontyModSub<N>(t, out, rp);
  std::copy(t, t + N, out);
}

/**
 * @brief Multiply two values in Montgomery representation.
 * @tparam N the number of limbs in the valus to multiply.
 * @param out the first operand and destiantion of result.
 * @param op the second operand.
 * @param rp reduction parameters.
 *
 * This function performs an <i>interleaved</i> Montgomery modular
 * multiplication.
 */
template <std::size_t N>
void MontyModMul(mp_limb_t* out, const mp_limb_t* op, const RedParams<N> rp) {
  mp_limb_t u[N + 1] = {0};

  for (std::size_t i = 0; i < N; ++i) {
    const auto c0 = mpn_addmul_1(u, op, N, out[i]);
    const auto q = rp.mc[0] * u[0];
    const auto c1 = mpn_addmul_1(u, rp.prime, N, q);
    u[N] += c1 + c0;
    std::copy(u + 1, u + N + 1, u);
    u[N] = ((c1 & c0) | ((c1 | c0) & ~u[N])) >> (SCL_BITS_PER_LIMB - 1);
  }

  std::copy(u, u + N, out);
  if (u[N] || mpn_cmp(out, rp.prime, N) >= 0) {
    mpn_sub_n(out, out, rp.prime, N);
  }
}

/**
 * @brief Square a value in Montgomery representation.
 * @tparam N the number of limbs in the value to square.
 * @param out the output.
 * @param op the operand to square.
 * @param rp reduction parameters.
 */
template <std::size_t N>
void MontyModSqr(mp_limb_t* out, const mp_limb_t* op, const RedParams<N> rp) {
  mp_limb_t res[2 * N];
  mpn_sqr(res, op, N);
  MontyRedc<N>(res, rp);
  std::copy(res, res + N, out);
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
 * @brief Modular exponentation.
 * @tparam N the number of limbs in the base.
 * @param out output. Must initially be equal to 1 in montgomery form.
 * @param base the base.
 * @param exp the exponent.
 * @param rp reduction parameters.
 */
template <std::size_t N>
void MontyModExp(mp_limb_t* out,
                 const mp_limb_t* base,
                 const mp_limb_t* exp,
                 const RedParams<N> rp) {
  auto n = mpn_sizeinbase(exp, N, 2);
  for (std::size_t i = n; i-- > 0;) {
    MontyModSqr<N>(out, out, rp);
    if (TestBit(exp, i)) {
      MontyModMul<N>(out, base, rp);
    }
  }
}

/**
 * @brief Compute a modular inverse.
 * @tparam N the number of limbs in the value to invert.
 * @param out output destination.
 * @param op the value to invert.
 * @param prime_minus_2 \p rp.prime minus 2.
 * @param rp reduction parameters.
 *
 * This function computes a modular inverse using Fermats little thereom. The \p
 * prime_minus_2 argument is assumed to be \f$rp.prime - 2\f$.
 */
template <std::size_t N>
void MontyModInv(mp_limb_t* out,
                 const mp_limb_t* op,
                 const mp_limb_t* prime_minus_2,
                 const RedParams<N> rp) {
  if (mpn_zero_p(op, N)) {
    throw std::invalid_argument("0 not invertible modulo prime");
  }

  MontyModExp<N>(out, op, prime_minus_2, rp);
}

/**
 * @brief Compute a comparison between two values.
 * @tparam N the number of limbs in the values to convert.
 * @return a value x such that <code>R(x, 0) <==> R(lhs, rhs)</code>.
 */
template <std::size_t N>
int CompareValues(const mp_limb_t* lhs, const mp_limb_t* rhs) {
  return mpn_cmp(lhs, rhs, N);
}

/**
 * @brief Deserialize a value and convert to montgomery form.
 * @tparam N the number of limbs in the value to convert.
 * @param out output destination.
 * @param src where to read the value from.
 * @param rp reduction parameters.
 */
template <std::size_t N>
void MontyFromBytes(mp_limb_t* out,
                    const unsigned char* src,
                    const RedParams<N> rp) {
  for (int i = N - 1; i >= 0; --i) {
    for (int j = SCL_BYTES_PER_LIMB - 1; j >= 0; --j) {
      out[i] |= static_cast<mp_limb_t>(*src++) << (j * 8);
    }
  }

  MontyIn<N>(out, rp);
}

/**
 * @brief Write a value in montgomery form to a buffer.
 * @tparam N the number of limbs in value to convert.
 * @param dest the output buffer.
 * @param src the input value.
 * @param rp reduction parameters.
 */
template <std::size_t N>
void MontyToBytes(unsigned char* dest,
                  const mp_limb_t* src,
                  const RedParams<N> rp) {
  mp_limb_t padded[2 * N] = {0};
  std::copy(src, src + N, padded);
  MontyRedc<N>(padded, rp);

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
 * @return the position of the first non-zero character.
 *
 * This method is used handle a string representation of a number with leading
 * zeros.
 */
std::size_t FindFirstNonZero(const std::string& s);

/**
 * @brief Convert a value in Montgomery representation to a string.
 * @tparam N the number of limbs in the value to convert.
 * @param val the value to convert.
 * @param rp reduction parameters used to convert \p val out of Montgomery form.
 * @return \p val as a string.
 */
template <std::size_t N>
std::string MontyToString(const mp_limb_t* val, const RedParams<N> rp) {
  mp_limb_t padded[2 * N] = {0};
  std::copy(val, val + N, padded);
  MontyRedc<N>(padded, rp);

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
 * @tparam N the number of limbs in the value to convert.
 * @param out the output destination.
 * @param str the string to read the output from.
 * @param rp reduction parameters used to convert out into Montgomery form.
 */
template <std::size_t N>
void MontyFromString(mp_limb_t* out,
                     const std::string& str,
                     const RedParams<N> rp) {
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
    MontyIn<N>(out, rp);
  }
}

#undef SCL_BITS_PER_LIMB
#undef SCL_BYTES_PER_LIMB

}  // namespace scl::math

#endif  // SCL_MATH_OPS_GMP_FF_H
