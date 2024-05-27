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

#include "scl/math/fields/mersenne61.h"

#include <cstring>
#include <string>

#include "./small_ff.h"
#include "scl/math/fields/ff_ops.h"
#include "scl/util/str.h"

using namespace scl;

using u64 = std::uint64_t;
using u128 = __uint128_t;

// The prime p = 2^61 - 1
static const u64 p = 0x1FFFFFFFFFFFFFFF;

using Mersenne61 = scl::math::ff::Mersenne61;

template <>
void math::ff::convertTo<Mersenne61>(u64& out, const int value) {
  out = value < 0 ? value + p : value;
}

template <>
void math::ff::convertTo<Mersenne61>(u64& out, const std::string& src) {
  out = util::fromHexString<u64>(src);
  out = out % p;
}

template <>
void math::ff::add<Mersenne61>(u64& out, const u64& op) {
  details::modAdd(out, op, p);
}

template <>
void math::ff::subtract<Mersenne61>(u64& out, const u64& op) {
  details::modSub(out, op, p);
}

template <>
void math::ff::multiply<Mersenne61>(u64& out, const u64& op) {
  u128 z = (u128)out * op;
  u64 a = z >> 61;
  u64 b = (u64)z;

  a |= b >> 61;
  b &= p;

  details::modAdd(a, b, p);
  out = a;
}

template <>
void math::ff::negate<Mersenne61>(u64& out) {
  details::modNeg(out, p);
}

template <>
void math::ff::invert<Mersenne61>(u64& out) {
  details::modInv<u64, std::int64_t>(out, out, p);
}

template <>
bool math::ff::equal<Mersenne61>(const u64& in1, const u64& in2) {
  return in1 == in2;
}

template <>
void math::ff::fromBytes<Mersenne61>(u64& dest, const unsigned char* src) {
  dest = *(const u64*)src;
  dest = dest % p;
}

template <>
void math::ff::toBytes<Mersenne61>(unsigned char* dest, const u64& src) {
  std::memcpy(dest, &src, sizeof(u64));
}

template <>
std::string math::ff::toString<Mersenne61>(const u64& in) {
  return util::toHexString(in);
}
