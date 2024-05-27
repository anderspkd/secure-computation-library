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

#include "scl/math/fields/secp256k1_scalar.h"

#include <array>
#include <cstddef>

#include <gmp.h>

#include "./secp256k1_helpers.h"
#include "scl/math/ff.h"
#include "scl/math/fields/ff_ops.h"
#include "scl/math/fields/ff_ops_gmp.h"

using namespace scl;

using Field = math::ff::Secp256k1Scalar;
using Elem = Field::ValueType;

constexpr static std::size_t NUM_LIMBS = std::tuple_size<Elem>{};

#define SCL_COPY(out, in, size)                \
  do {                                         \
    for (std::size_t i = 0; i < (size); ++i) { \
      *((out) + i) = *((in) + i);              \
    }                                          \
  } while (0)

template <>
math::Number math::order<math::FF<Field>>() {
  return Number::fromString(
      "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141");
}

static const math::ff::RedParams<NUM_LIMBS> RD = {
    // Prime
    {
        0xBFD25E8CD0364141,  //
        0xBAAEDCE6AF48A03B,  //
        0xFFFFFFFFFFFFFFFE,  //
        0xFFFFFFFFFFFFFFFF   //
    },
    // Montgomery constant
    {
        0x4B0DFF665588B13F,  //
        0x50A51AC834B9EC24,  //
        0x897F30C127CFAB5E,  //
        0xD9E8890D6494EF93   //
    }};

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
  { 0x402DA1732FC9BEBF, 0x4551231950B75FC4, 0x1, 0 }

template <>
void math::ff::invert<Field>(Elem& out) {
  static const mp_limb_t PRIME_MINUS_2[NUM_LIMBS] = {
      0xBFD25E8CD036413F,  //
      0xBAAEDCE6AF48A03B,  //
      0xFFFFFFFFFFFFFFFE,  //
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

namespace {

bool testBit(const mp_limb_t* in, std::size_t pos) {
  const auto bits_per_limb = static_cast<std::size_t>(mp_bits_per_limb);
  const auto limb = pos / bits_per_limb;
  const auto limb_pos = pos % bits_per_limb;
  return ((in[limb] >> limb_pos) & 1) == 1;
}

}  // namespace

math::FF<Field> math::details::fromMonty(const FF<Field>& x) {
  mp_limb_t padded[2 * NUM_LIMBS] = {0};
  SCL_COPY(padded, PTR(x.value()), NUM_LIMBS);
  montyRedc<NUM_LIMBS>(padded, RD);

  FF<Field> r;
  SCL_COPY(PTR(r.value()), padded, NUM_LIMBS);

  return r;
}

namespace {

void add1(mp_limb_t* out) {
  static const mp_limb_t one[NUM_LIMBS] = {1, 0, 0, 0};
  mpn_add_n(out, out, one, NUM_LIMBS);
}

void sub1(mp_limb_t* out) {
  static const mp_limb_t one[NUM_LIMBS] = {1, 0, 0, 0};
  mpn_sub_n(out, out, one, NUM_LIMBS);
}

}  // namespace

// Compute a NAF encoding of a field element using the simpel algorithm provided
// here: https://en.wikipedia.org/wiki/Non-adjacent_form#Converting_to_NAF
math::details::NafEncoding<Field> math::details::toNaf(const FF<Field>& x) {
  using NafEnc = math::details::NafEncoding<Field>;

  auto val = fromMonty(x).value();

  std::array<NafEnc::Value, NafEnc::MAX_SIZE> z;
  std::size_t i = 0;

  while (!mpn_zero_p(PTR(val), NUM_LIMBS)) {
    // check if val is odd
    if (::testBit(PTR(val), 0)) {
      // check if val is 1 or 3 mod 4
      if (::testBit(PTR(val), 1)) {
        z[i] = NafEnc::Value::createNeg();
        add1(PTR(val));
      } else {
        z[i] = NafEnc::Value::createPos();
        sub1(PTR(val));
      }
    } else {
      z[i] = NafEnc::Value::createZero();
    }

    i++;
    mpn_rshift(PTR(val), PTR(val), NUM_LIMBS, 1);
  }

  return {z, i};
}

#undef ONE
