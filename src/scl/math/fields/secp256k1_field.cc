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

#include "scl/math/fields/secp256k1_field.h"

#include <array>
#include <sstream>

#include "./secp256k1_helpers.h"
#include "scl/math/ff.h"
#include "scl/math/fields/ff_ops.h"
#include "scl/math/fields/ff_ops_gmp.h"
#include "scl/math/number.h"

using namespace scl;

using Field = math::ff::Secp256k1Field;
using Elem = Field::ValueType;

#define NUM_LIMBS 4

#define SCL_COPY(out, in, size)                \
  do {                                         \
    for (std::size_t i = 0; i < (size); ++i) { \
      *((out) + i) = *((in) + i);              \
    }                                          \
  } while (0)

static const math::ff::RedParams<NUM_LIMBS> RD = {
    // Prime
    {
        0xFFFFFFFEFFFFFC2F,  //
        0xFFFFFFFFFFFFFFFF,  //
        0xFFFFFFFFFFFFFFFF,  //
        0xFFFFFFFFFFFFFFFF   //
    },
    // Montgomery constant
    {
        0xD838091DD2253531,  //
        0xBCB223FEDC24A059,  //
        0x9C46C2C295F2B761,  //
        0xC9BD190515538399   //
    }};

template <>
math::Number math::order<math::FF<Field>>() {
  return Number::fromString(
      "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F");
}

// The internal data type is an STL array, but gmp expects pointers.
#define PTR(X) (X).data()

template <>
void math::ff::convertTo<Field>(Elem& out, const int value) {
  out = {0};
  montyInFromInt<NUM_LIMBS>(PTR(out), value, RD);
}

template <>
void math::ff::convertTo<Field>(Elem& out, const std::string& src) {
  out = {0};
  montyFromString<NUM_LIMBS>(PTR(out), src, RD);
}

template <>
void math::ff::add<Field>(Elem& out, const Elem& op) {
  montyModAdd<NUM_LIMBS>(PTR(out), PTR(op), RD);
}

template <>
void math::ff::subtract<Field>(Elem& out, const Elem& op) {
  montyModSub<NUM_LIMBS>(PTR(out), PTR(op), RD);
}

template <>
void math::ff::negate<Field>(Elem& out) {
  montyModNeg<NUM_LIMBS>(PTR(out), RD);
}

template <>
void math::ff::multiply<Field>(Elem& out, const Elem& op) {
  montyModMul<NUM_LIMBS>(PTR(out), PTR(op), RD);
}

#define ONE \
  { 0x1000003D1, 0, 0, 0 }

template <>
void math::ff::invert<Field>(Elem& out) {
  static const mp_limb_t PRIME_MINUS_2[NUM_LIMBS] = {
      0xFFFFFFFEFFFFFC2D,  //
      0xFFFFFFFFFFFFFFFF,  //
      0xFFFFFFFFFFFFFFFF,  //
      0xFFFFFFFFFFFFFFFF   //
  };

  Elem res = ONE;
  montyModInv<NUM_LIMBS>(PTR(res), PTR(out), PRIME_MINUS_2, RD);
  out = res;
}

template <>
bool math::ff::equal<Field>(const Elem& in1, const Elem& in2) {
  return compareValues<NUM_LIMBS>(PTR(in1), PTR(in2)) == 0;
}

template <>
void math::ff::fromBytes<Field>(Elem& dest, const unsigned char* src) {
  montyFromBytes<NUM_LIMBS>(PTR(dest), src, RD);
}

template <>
void math::ff::toBytes<Field>(unsigned char* dest, const Elem& src) {
  montyToBytes<NUM_LIMBS>(dest, PTR(src), RD);
}

template <>
std::string math::ff::toString<Field>(const Elem& in) {
  return montyToString<NUM_LIMBS>(PTR(in), RD);
}

bool math::details::isSmaller(const FF<Field>& lhs, const FF<Field>& rhs) {
  auto c = ff::compareValues<NUM_LIMBS>(PTR(lhs.value()), PTR(rhs.value()));
  return c <= 0;
}

math::FF<Field> math::details::sqrt(const FF<Field>& x) {
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
  montyModExp<NUM_LIMBS>(PTR(res), PTR(x.value()), e, RD);
  out.value() = res;
  return out;
}

#undef ONE
