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

#include <array>
#include <sstream>

#include "./secp256k1_helpers.h"
#include "scl/math/curves/secp256k1.h"
#include "scl/math/ff_ops.h"
#include "scl/math/number.h"
#include "scl/math/ops_gmp_ff.h"

using Field = scl::math::Secp256k1::Field;
using Elem = Field::ValueType;

#define NUM_LIMBS 4

#define SCL_COPY(out, in, size)                \
  do {                                         \
    for (std::size_t i = 0; i < (size); ++i) { \
      *((out) + i) = *((in) + i);              \
    }                                          \
  } while (0)

// The prime modulus p
static const mp_limb_t kPrime[] = {
    0xFFFFFFFEFFFFFC2F,  //
    0xFFFFFFFFFFFFFFFF,  //
    0xFFFFFFFFFFFFFFFF,  //
    0xFFFFFFFFFFFFFFFF   //
};

// n' such that 2^{256} * a + kPrime * n' == 1
static const mp_limb_t kMontyN[] = {
    0x27C7F6E22DDACACF,  //
    0x434DDC0123DB5FA6,  //
    0x63B93D3D6A0D489E,  //
    0x3642E6FAEAAC7C66   //
};

// The internal data type is an STL array, but gmp expects pointers.
#define PTR(X) (X).data()

template <>
void scl::math::FieldConvertIn<Field>(Elem& out, const int value) {
  out = {0};
  MontyInFromInt<NUM_LIMBS>(PTR(out), value, kPrime);
}

template <>
void scl::math::FieldAdd<Field>(Elem& out, const Elem& op) {
  MontyModAdd<NUM_LIMBS>(PTR(out), PTR(op), kPrime);
}

template <>
void scl::math::FieldSubtract<Field>(Elem& out, const Elem& op) {
  MontyModSub<NUM_LIMBS>(PTR(out), PTR(op), kPrime);
}

template <>
void scl::math::FieldNegate<Field>(Elem& out) {
  MontyModNeg<NUM_LIMBS>(PTR(out), kPrime);
}

template <>
void scl::math::FieldMultiply<Field>(Elem& out, const Elem& op) {
  MontyModMul<NUM_LIMBS>(PTR(out), PTR(op), kPrime, kMontyN);
}

#define ONE \
  { 0x1000003D1, 0, 0, 0 }

template <>
void scl::math::FieldInvert<Field>(Elem& out) {
  static const mp_limb_t kPrimeMinus2[NUM_LIMBS] = {
      0xFFFFFFFEFFFFFC2D,  //
      0xFFFFFFFFFFFFFFFF,  //
      0xFFFFFFFFFFFFFFFF,  //
      0xFFFFFFFFFFFFFFFF   //
  };

  Elem res = ONE;
  MontyModInv<NUM_LIMBS>(PTR(res), PTR(out), kPrime, kPrimeMinus2, kMontyN);
  out = res;
}

template <>
bool scl::math::FieldEqual<Field>(const Elem& in1, const Elem& in2) {
  return CompareValues<NUM_LIMBS>(PTR(in1), PTR(in2)) == 0;
}

template <>
void scl::math::FieldFromBytes<Field>(Elem& dest, const unsigned char* src) {
  MontyFromBytes<NUM_LIMBS>(PTR(dest), src, kPrime);
}

template <>
void scl::math::FieldToBytes<Field>(unsigned char* dest, const Elem& src) {
  MontyToBytes<NUM_LIMBS>(dest, PTR(src), kPrime, kMontyN);
}

template <>
void scl::math::FieldFromString<Field>(Elem& out, const std::string& src) {
  out = {0};
  MontyFromString<NUM_LIMBS>(PTR(out), kPrime, src);
}

template <>
std::string scl::math::FieldToString<Field>(const Elem& in) {
  return MontyToString<NUM_LIMBS>(PTR(in), kPrime, kMontyN);
}

bool scl::math::FFAccess<Field>::IsSmaller(
    const scl::math::FF<Secp256k1::Field>& lhs,
    const scl::math::FF<Secp256k1::Field>& rhs) {
  auto c = CompareValues<NUM_LIMBS>(PTR(lhs.m_value), PTR(rhs.m_value));
  return c <= 0;
}

scl::math::FF<Field> scl::math::FFAccess<Field>::ComputeSqrt(
    const scl::math::FF<Field>& x) {
  // (p + 1) / 4. We assume the input is a square mod p, so x^{e} gives a square
  // root of x.
  static const mp_limb_t e[NUM_LIMBS] = {
      0xFFFFFFFFBFFFFF0C,  //
      0xFFFFFFFFFFFFFFFF,  //
      0xFFFFFFFFFFFFFFFF,  //
      0x3FFFFFFFFFFFFFFF   //
  };

  FF<Field> out;
  Elem res = ONE;
  MontyModExp<NUM_LIMBS>(PTR(res), PTR(x.m_value), e, kPrime, kMontyN);
  out.m_value = res;
  return out;
}

#undef ONE
