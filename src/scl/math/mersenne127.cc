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

#include "scl/math/mersenne127.h"

#include <cstdint>
#include <cstring>

#include "scl/hex.h"
#include "scl/math/ff_ops.h"
#include "scl/math/hex_util.h"
#include "scl/math/small_ff.h"

using u64 = std::uint64_t;
using u128 = __uint128_t;

// The prime p = 2^127 - 1.
static const u128 p = (((u128)0x7FFFFFFFFFFFFFFF) << 64) | 0xFFFFFFFFFFFFFFFF;

using Mersenne127 = scl::Mersenne127;

template <>
void scl::details::convertTo<Mersenne127>(u128& out, const int value) {
  out = value < 0 ? value + p : value;
}

template <>
void scl::details::convertTo<Mersenne127>(u128& out, const std::string& src) {
  out = fromHexString<u128>(src);
  out = out % p;
}

template <>
void scl::details::add<Mersenne127>(u128& out, const u128& op) {
  modAdd(out, op, p);
}

template <>
void scl::details::subtract<Mersenne127>(u128& out, const u128& op) {
  details::modSub(out, op, p);
}

namespace {

struct u256 {
  u128 high;
  u128 low;
};

//  https://cp-algorithms.com/algebra/montgomery_multiplication.html
u256 multiplyFull(const u128 x, const u128 y) {
  u64 a = x >> 64;
  u64 b = x;
  u64 c = y >> 64;
  u64 d = y;
  // (a*2^64 + b) * (c*2^64 + d) =
  // (a*c) * 2^128 + (a*d + b*c)*2^64 + (b*d)
  u128 ac = (u128)a * c;
  u128 ad = (u128)a * d;
  u128 bc = (u128)b * c;
  u128 bd = (u128)b * d;

  u128 carry = (u128)(u64)ad + (u128)(u64)bc + (bd >> 64U);
  u128 high = ac + (ad >> 64U) + (bc >> 64U) + (carry >> 64U);
  u128 low = (ad << 64U) + (bc << 64U) + bd;

  return {high, low};
}

}  // namespace

template <>
void scl::details::multiply<Mersenne127>(u128& out, const u128& op) {
  u256 z = multiplyFull(out, op);
  out = z.high << 1;
  u128 b = z.low;

  out |= b >> 127;
  b &= p;

  details::modAdd(out, b, p);
}

template <>
void scl::details::negate<Mersenne127>(u128& out) {
  modNeg(out, p);
}

template <>
void scl::details::invert<Mersenne127>(u128& out) {
  modInv<u128, __int128_t>(out, out, p);
}

template <>
bool scl::details::equal<Mersenne127>(const u128& in1, const u128& in2) {
  return in1 == in2;
}

template <>
void scl::details::fromBytes<Mersenne127>(u128& dest,
                                          const unsigned char* src) {
  dest = *(const u128*)src;
  dest = dest % p;
}

template <>
void scl::details::toBytes<Mersenne127>(unsigned char* dest, const u128& src) {
  std::memcpy(dest, &src, sizeof(u128));
}

template <>
std::string scl::details::toString<Mersenne127>(const u128& in) {
  return toHexString(in);
}
