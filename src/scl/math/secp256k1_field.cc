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

#include <array>
#include <sstream>

#include "./ops_gmp_ff.h"
#include "./secp256k1_extras.h"
#include "scl/math/curves/secp256k1.h"
#include "scl/math/ff_ops.h"
#include "scl/math/number.h"

using Field = scl::details::Secp256k1::Field;
using Elem = Field::ValueType;

#define NUM_LIMBS 4

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
void scl::details::FieldConvertIn<Field>(Elem& out, const int value) {
  out = {0};
  FromInt<NUM_LIMBS>(PTR(out), value, kPrime);
}

template <>
void scl::details::FieldAdd<Field>(Elem& out, const Elem& op) {
  ModAdd<NUM_LIMBS>(PTR(out), PTR(op), kPrime);
}

template <>
void scl::details::FieldSubtract<Field>(Elem& out, const Elem& op) {
  ModSub<NUM_LIMBS>(PTR(out), PTR(op), kPrime);
}

template <>
void scl::details::FieldNegate<Field>(Elem& out) {
  ModNeg<NUM_LIMBS>(PTR(out), kPrime);
}

template <>
void scl::details::FieldMultiply<Field>(Elem& out, const Elem& op) {
  ModMul<NUM_LIMBS>(PTR(out), PTR(op), kPrime, kMontyN);
}

#define ONE \
  { 0x1000003D1, 0, 0, 0 }

template <>
void scl::details::FieldInvert<Field>(Elem& out) {
  static const mp_limb_t kPrimeMinus2[NUM_LIMBS] = {
      0xFFFFFFFEFFFFFC2D,  //
      0xFFFFFFFFFFFFFFFF,  //
      0xFFFFFFFFFFFFFFFF,  //
      0xFFFFFFFFFFFFFFFF   //
  };

  Elem res = ONE;
  ModInvFermat<NUM_LIMBS>(PTR(res), PTR(out), kPrime, kPrimeMinus2, kMontyN);
  out = res;
}

template <>
bool scl::details::FieldEqual<Field>(const Elem& in1, const Elem& in2) {
  return CompareValues<NUM_LIMBS>(PTR(in1), PTR(in2)) == 0;
}

template <>
void scl::details::FieldFromBytes<Field>(Elem& dest, const unsigned char* src) {
  ValueFromBytes<NUM_LIMBS>(PTR(dest), src, kPrime);
}

template <>
void scl::details::FieldToBytes<Field>(unsigned char* dest, const Elem& src) {
  ValueToBytes<NUM_LIMBS>(dest, PTR(src), kPrime, kMontyN);
}

template <>
void scl::details::FieldFromString<Field>(Elem& out, const std::string& src) {
  out = {0};
  FromString<NUM_LIMBS>(PTR(out), kPrime, src);
}

template <>
std::string scl::details::FieldToString<Field>(const Elem& in) {
  return ToString<NUM_LIMBS>(PTR(in), kPrime, kMontyN);
}

bool scl::SCL_FF_Extras<Field>::IsSmaller(
    const scl::FF<details::Secp256k1::Field>& lhs,
    const scl::FF<details::Secp256k1::Field>& rhs) {
  auto c = details::CompareValues<NUM_LIMBS>(PTR(lhs.mValue), PTR(rhs.mValue));
  return c <= 0;
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
  Elem res = ONE;
  details::ModExp<NUM_LIMBS>(PTR(res), PTR(x.mValue), e, kPrime, kMontyN);
  out.mValue = res;
  return out;
}

#undef ONE
