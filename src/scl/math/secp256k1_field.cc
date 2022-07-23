/**
 * @file secp256k1_field.cc
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

#include <gmp.h>

#include <sstream>

#include "./secp256k1_extras.h"
#include "scl/math/curves/secp256k1.h"
#include "scl/math/ff_ops.h"

using Field = scl::details::Secp256k1::Field;
using Elem = std::array<mp_limb_t, 4>;

#define NUM_LIMBS 4
#define BITS_PER_LIMB static_cast<std::size_t>(mp_bits_per_limb)
#define BYTES_PER_LIMB sizeof(mp_limb_t)

// The prime modulus p
static const mp_limb_t kPrime[NUM_LIMBS] = {
    0xFFFFFFFEFFFFFC2F,  //
    0xFFFFFFFFFFFFFFFF,  //
    0xFFFFFFFFFFFFFFFF,  //
    0xFFFFFFFFFFFFFFFF   //
};

// The internal data type is an STL array, but gmp expects pointers.
#define PTR(X) (X).data()

// copy two field elements in their internal representation.
#define COPY(out, in)                             \
  do {                                            \
    for (std::size_t i = 0; i < NUM_LIMBS; ++i) { \
      *((out) + i) = *((in) + i);                 \
    }                                             \
  } while (0)

namespace {

// Convert a value of NUM_LIMBS limbs into montgomery representation.
void ToMonty(mp_limb_t* val) {
  mp_limb_t qp[NUM_LIMBS + 1];
  mp_limb_t shift[2 * NUM_LIMBS] = {0};
  // multiply val by 2^{256}
  COPY(shift + NUM_LIMBS, val);
  // compute (val * 2^{256}) mod p
  mpn_tdiv_qr(qp, val, 0, shift, 2 * NUM_LIMBS, kPrime, NUM_LIMBS);
}

// Perform a montgomery reduction on a value of size 2 * NUM_LIMBS limbs.
// https://cp-algorithms.com/algebra/montgomery_multiplication.html#montgomery-reduction
void Redc(mp_limb_t* val) {
  // n' such that 2^{256} * a + kPrime * n' == 1
  static const mp_limb_t kMontyN[NUM_LIMBS] = {
      0x27C7F6E22DDACACF,  //
      0x434DDC0123DB5FA6,  //
      0x63B93D3D6A0D489E,  //
      0x3642E6FAEAAC7C66   //
  };

  mp_limb_t q[2 * NUM_LIMBS];
  // TODO: This multiplication can be optimized because we're only interested in
  // the result mod r = 2^{256}.
  mpn_mul_n(q, val, kMontyN, NUM_LIMBS);
  mp_limb_t c[2 * NUM_LIMBS];
  mpn_mul_n(c, q, kPrime, NUM_LIMBS);
  auto borrow = mpn_sub_n(c, val, c, 2 * NUM_LIMBS);

  COPY(val, c + NUM_LIMBS);

  if (borrow) {
    mpn_add_n(val, val, kPrime, NUM_LIMBS);
  }
}

}  // namespace

template <>
void scl::details::FieldConvertIn<Field>(Elem& out, const int value) {
  out = {0};
  out[0] = std::abs(value);
  if (value < 0) {
    mpn_sub_n(PTR(out), kPrime, PTR(out), NUM_LIMBS);
  }
  ToMonty(PTR(out));
}

template <>
void scl::details::FieldAdd<Field>(Elem& out, const Elem& op) {
  auto carry = mpn_add_n(PTR(out), PTR(out), PTR(op), NUM_LIMBS);
  if (carry || mpn_cmp(PTR(out), kPrime, NUM_LIMBS) >= 0) {
    mpn_sub_n(PTR(out), PTR(out), kPrime, NUM_LIMBS);
  }
}

template <>
void scl::details::FieldSubtract<Field>(Elem& out, const Elem& op) {
  auto carry = mpn_sub_n(PTR(out), PTR(out), PTR(op), NUM_LIMBS);
  if (carry) {
    mpn_add_n(PTR(out), PTR(out), kPrime, NUM_LIMBS);
  }
}

template <>
void scl::details::FieldNegate<Field>(Elem& out) {
  mpn_sub_n(PTR(out), kPrime, PTR(out), NUM_LIMBS);
}

template <>
void scl::details::FieldMultiply<Field>(Elem& out, const Elem& op) {
  mp_limb_t res[2 * NUM_LIMBS];
  mpn_mul_n(res, PTR(out), PTR(op), NUM_LIMBS);
  Redc(res);
  COPY(PTR(out), res);
}

namespace {

bool TestBit(const mp_limb_t* v, std::size_t pos) {
  auto limb = pos / BITS_PER_LIMB;
  auto bit = pos % BITS_PER_LIMB;
  return ((v[limb] >> bit) & 1) == 1;
}

std::size_t BitLengthOfNonZero(const mp_limb_t* v) {
  return mpn_scan1(v, NUM_LIMBS * BITS_PER_LIMB);
}

Elem Exp(const Elem& x, const mp_limb_t* e) {
  // 1 in montgomery representation.
  Elem res = {0x1000003D1, 0, 0, 0};
  auto n = BitLengthOfNonZero(e);

  for (int i = n - 1; i >= 0; --i) {
    scl::details::FieldMultiply<Field>(res, res);
    if (TestBit(e, i)) {
      scl::details::FieldMultiply<Field>(res, x);
    }
  }
  return res;
}

}  // namespace

template <>
void scl::details::FieldInvert<Field>(Elem& out) {
  static const mp_limb_t kPrimeMinus2[NUM_LIMBS] = {
      0xFFFFFFFEFFFFFC2D,  //
      0xFFFFFFFFFFFFFFFF,  //
      0xFFFFFFFFFFFFFFFF,  //
      0xFFFFFFFFFFFFFFFF   //
  };

  if (mpn_zero_p(PTR(out), NUM_LIMBS))
    throw std::invalid_argument("0 not invertible modulo prime");

  out = Exp(out, kPrimeMinus2);
}

template <>
bool scl::details::FieldEqual<Field>(const Elem& first, const Elem& second) {
  return mpn_cmp(PTR(first), PTR(second), NUM_LIMBS) == 0;
}

namespace {

void ReadLimb(mp_limb_t& lmb, const unsigned char* bytes) {
  std::size_t c = 0;
  lmb = 0;
  for (std::size_t i = 0; i < BITS_PER_LIMB; i += 8) {
    lmb |= static_cast<mp_limb_t>(bytes[c++]) << i;
  }
}

}  // namespace

template <>
void scl::details::FieldFromBytes<Field>(Elem& out, const unsigned char* src) {
  for (std::size_t i = 0; i < NUM_LIMBS; ++i) {
    ReadLimb(out[i], src + i * BYTES_PER_LIMB);
  }
}

namespace {

std::size_t FindFirstNonZero(const std::string& s) {
  int n = 0;
  for (const auto c : s) {
    if (c != '0') {
      return n;
    }
    n++;
  }
  return n;
}

}  // namespace

template <>
std::string scl::details::FieldToString<Field>(const Elem& element) {
  // Pad the element we are about to print to ensure that it's the right size
  // for Redc.
  mp_limb_t padded[2 * NUM_LIMBS] = {0};
  COPY(padded, PTR(element));
  Redc(padded);
  static const char* kHexChars = "0123456789abcdef";
  std::stringstream ss;
  for (int i = NUM_LIMBS - 1; i >= 0; --i) {
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

namespace {

void ReduceNaive(mp_limb_t* out) {
  mp_limb_t qp[1];
  mpn_tdiv_qr(qp, out, 0, out, NUM_LIMBS, kPrime, NUM_LIMBS);
}

}  // namespace

template <>
void scl::details::FieldFromString<Field>(Elem& out, const std::string& str) {
  out = {0};
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
    // The input might be several times larger than the modulus, so a naive
    // reduction (i.e., computation mod p) is needed here before we transform it
    // into montgomery form.
    ReduceNaive(PTR(out));
    ToMonty(PTR(out));
  }
}

bool scl::SCL_FF_Extras<Field>::IsSmaller(
    const scl::FF<details::Secp256k1::Field>& lhs,
    const scl::FF<details::Secp256k1::Field>& rhs) {
  return mpn_cmp(PTR(lhs.mValue), PTR(rhs.mValue), 4) <= 0;
}

scl::FF<Field> scl::SCL_FF_Extras<Field>::ComputeSqrt(const scl::FF<Field>& x) {
  // (p + 1) / 4. We assume the input is a square mod p, so x^{e} gives a square
  // root of x.
  static const mp_limb_t e[NUM_LIMBS] = {
      0xFFFFFFFFBFFFFF0C,  //
      0xFFFFFFFFFFFFFFFF,  //
      0xFFFFFFFFFFFFFFFF,  //
      0x3FFFFFFFFFFFFFFF   //
  };

  scl::FF<Field> out;
  auto r = Exp(x.mValue, e);
  out.mValue = r;
  return out;
}
