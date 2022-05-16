/**
 * @file mersenne127.cc
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

#include <cstring>
#include <sstream>

#include "./simple_ff.h"
#include "scl/math/fields.h"
#include "scl/math/str.h"

using u64 = std::uint64_t;
using u128 = __uint128_t;

static const u128 p = (((u128)0x7FFFFFFFFFFFFFFF) << 64) | 0xFFFFFFFFFFFFFFFF;
using _ = scl::details::Mersenne127;

u128 _::FromInt(int v) { return v < 0 ? v + p : v; }

void _::Add(u128 &t, const u128 &v) { ModAdd(t, v, p); }

void _::Subtract(u128 &t, const u128 &v) { ModSub(t, v, p); }

void _::Negate(u128 &t) { ModNeg(t, p); }

bool _::Equal(const u128 &a, const u128 &b) { return a == b; }

void _::Invert(u128 &t) { ModInv<u128, __int128_t>(t, t, p); }

void _::FromString(u128 &dest, const std::string &str,
                   enum scl::NumberBase base) {
  FromStringSimpleType(dest, str, base);
}

struct u256 {
  u128 high;
  u128 low;
};

//  https://cp-algorithms.com/algebra/montgomery_multiplication.html
static inline u256 MultiplyFull(const u128 x, const u128 y) {
  u64 a = x >> 64, b = x;
  u64 c = y >> 64, d = y;
  // (a*2^64 + b) * (c*2^64 + d) =
  // (a*c) * 2^128 + (a*d + b*c)*2^64 + (b*d)
  u128 ac = (u128)a * c;
  u128 ad = (u128)a * d;
  u128 bc = (u128)b * c;
  u128 bd = (u128)b * d;

  u128 carry = (u128)(u64)ad + (u128)(u64)bc + (bd >> 64u);
  u128 high = ac + (ad >> 64u) + (bc >> 64u) + (carry >> 64u);
  u128 low = (ad << 64u) + (bc << 64u) + bd;

  return {high, low};
}

void _::Multiply(u128 &t, const u128 &v) {
  u256 z = MultiplyFull(t, v);
  t = z.high << 1;
  u128 b = z.low;

  t |= b >> 127;
  b &= p;

  Add(t, b);
}

std::string _::ToString(const u128 &v) { return scl::details::ToString(v); }

void _::FromBytes(u128 &dest, const unsigned char *src) {
  dest = *(const u128 *)src;
  dest = dest % p;
}

void _::ToBytes(unsigned char *dest, const u128 &src) {
  std::memcpy(dest, (unsigned char *)&src, 16);
}
