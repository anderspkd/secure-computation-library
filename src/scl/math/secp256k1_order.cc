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
#include <cstddef>

#include <gmp.h>

#include "./secp256k1_helpers.h"
#include "scl/math/curves/secp256k1.h"
#include "scl/math/ec.h"
#include "scl/math/ff.h"
#include "scl/math/ff_ops.h"
#include "scl/math/ops_gmp_ff.h"

using Field = scl::math::Secp256k1::Order;
using Elem = Field::ValueType;

#define NUM_LIMBS 4

#define SCL_COPY(out, in, size)                \
  do {                                         \
    for (std::size_t i = 0; i < (size); ++i) { \
      *((out) + i) = *((in) + i);              \
    }                                          \
  } while (0)

static const mp_limb_t kPrime[] = {
    0xBFD25E8CD0364141,  //
    0xBAAEDCE6AF48A03B,  //
    0xFFFFFFFFFFFFFFFE,  //
    0xFFFFFFFFFFFFFFFF   //
};

static const mp_limb_t kMontyN[] = {
    0xB4F20099AA774EC1,  //
    0xAF5AE537CB4613DB,  //
    0x7680CF3ED83054A1,  //
    0x261776F29B6B106C   //
};

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
  { 0x402DA1732FC9BEBF, 0x4551231950B75FC4, 0x1, 0 }

template <>
void scl::math::FieldInvert<Field>(Elem& out) {
  static const mp_limb_t kPrimeMinus2[NUM_LIMBS] = {
      0xBFD25E8CD036413F,  //
      0xBAAEDCE6AF48A03B,  //
      0xFFFFFFFFFFFFFFFE,  //
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
std::string scl::math::FieldToString<Field>(const Elem& in) {
  return MontyToString<NUM_LIMBS>(PTR(in), kPrime, kMontyN);
}

template <>
void scl::math::FieldFromString<Field>(Elem& out, const std::string& src) {
  out = {0};
  MontyFromString<NUM_LIMBS>(PTR(out), kPrime, src);
}

std::size_t scl::math::FFAccess<Field>::HigestSetBit(
    const scl::math::FF<Field>& element) {
  return mpn_sizeinbase(PTR(element.mValue), NUM_LIMBS, 2);
}

bool scl::math::FFAccess<Field>::TestBit(const scl::math::FF<Field>& element,
                                         std::size_t pos) {
  const auto bits_per_limb = static_cast<std::size_t>(mp_bits_per_limb);
  const auto limb = pos / bits_per_limb;
  const auto limb_pos = pos % bits_per_limb;
  return ((element.mValue[limb] >> limb_pos) & 1) == 1;
}

scl::math::FF<Field> scl::math::FFAccess<Field>::FromMonty(
    const scl::math::FF<Field>& element) {
  mp_limb_t padded[2 * NUM_LIMBS] = {0};
  SCL_COPY(padded, PTR(element.mValue), NUM_LIMBS);
  MontyRedc<NUM_LIMBS>(padded, kPrime, kMontyN);

  FF<Field> r;
  SCL_COPY(PTR(r.mValue), padded, NUM_LIMBS);

  return r;
}

#undef ONE
