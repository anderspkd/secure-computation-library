/* SCL --- Secure Computation Library
 * Copyright (C) 2025 Anders Dalskov
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

#include "scl/math/mersenne61.h"

#include <cstring>
#include <string>

#include "./small_ff.h"
#include "scl/hex.h"
#include "scl/math/ff_ops.h"

using u64 = std::uint64_t;
using u128 = __uint128_t;

// The prime p = 2^61 - 1
static const u64 p = 0x1FFFFFFFFFFFFFFF;

using Mersenne61 = scl::Mersenne61;

template <>
void scl::details::convertTo<Mersenne61>(u64& out, const int value) {
  out = value < 0 ? value + p : value;
}

template <>
void scl::details::convertTo<Mersenne61>(u64& out, const std::string& src) {
  out = fromHexString<u64>(src);
  out = out % p;
}

template <>
void scl::details::add<Mersenne61>(u64& out, const u64& op) {
  modAdd(out, op, p);
}

template <>
void scl::details::subtract<Mersenne61>(u64& out, const u64& op) {
  modSub(out, op, p);
}

template <>
void scl::details::multiply<Mersenne61>(u64& out, const u64& op) {
  u128 z = (u128)out * op;
  u64 a = z >> 61;
  u64 b = (u64)z;

  a |= b >> 61;
  b &= p;

  modAdd(a, b, p);
  out = a;
}

template <>
void scl::details::negate<Mersenne61>(u64& out) {
  modNeg(out, p);
}

template <>
void scl::details::invert<Mersenne61>(u64& out) {
  modInv<u64, std::int64_t>(out, out, p);
}

template <>
bool scl::details::equal<Mersenne61>(const u64& in1, const u64& in2) {
  return in1 == in2;
}

template <>
void scl::details::fromBytes<Mersenne61>(u64& dest, const unsigned char* src) {
  dest = *(const u64*)src;
  dest = dest % p;
}

template <>
void scl::details::toBytes<Mersenne61>(unsigned char* dest, const u64& src) {
  std::memcpy(dest, &src, sizeof(u64));
}

template <>
std::string scl::details::toString<Mersenne61>(const u64& in) {
  return toHexString(in);
}
