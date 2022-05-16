/**
 * @file mersenne61.cc
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
#include <iostream>
#include <sstream>

#include "./simple_ff.h"
#include "scl/math/fields.h"
#include "scl/math/str.h"

using u64 = std::uint64_t;
using u128 = __uint128_t;

static const u64 p = 0x1FFFFFFFFFFFFFFF;
using _ = scl::details::Mersenne61;

u64 _::FromInt(int v) { return v < 0 ? v + p : v; }

void _::Add(u64& t, const u64& v) { ModAdd(t, v, p); }

void _::Subtract(u64& t, const u64& v) { ModSub(t, v, p); }

void _::Negate(u64& t) { ModNeg(t, p); }

bool _::Equal(const u64& a, const u64& b) {
  // This is fine since this method should never be called directly, but is
  // instead called by the surrounding FF object which makes sure to only pass
  // values that are in a valid range.
  return a == b;
}

void _::Invert(u64& t) { ModInv<u64, std::int64_t>(t, t, p); }

void _::FromString(u64& dest, const std::string& str,
                   enum scl::NumberBase base) {
  FromStringSimpleType(dest, str, base);
}

void _::Multiply(u64& t, const u64& v) {
  u128 z = (u128)t * v;
  u64 a = z >> 61;
  u64 b = (u64)z;

  a |= b >> 61;
  b &= p;

  Add(a, b);
  t = a;
}

std::string _::ToString(const u64& v) { return scl::details::ToString(v); }

void _::FromBytes(u64& dest, const unsigned char* src) {
  dest = *(const u64*)src;
  dest = dest % p;
}

void _::ToBytes(unsigned char* dest, const u64& src) {
  std::memcpy(dest, &src, sizeof(u64));
}
