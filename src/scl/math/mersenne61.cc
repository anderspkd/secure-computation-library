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

#include "scl/math/fields/mersenne61.h"

#include <cstring>
#include <iostream>
#include <sstream>

#include "./ops_small_fp.h"
#include "scl/math/ff_ops.h"
#include "scl/util/str.h"

using u64 = std::uint64_t;
using u128 = __uint128_t;

static const u64 p = 0x1FFFFFFFFFFFFFFF;
using Mersenne61 = scl::details::Mersenne61;

template <>
void scl::details::FieldConvertIn<Mersenne61>(u64& out, const int value) {
  out = value < 0 ? value + p : value;
}

template <>
void scl::details::FieldAdd<Mersenne61>(u64& out, const u64& op) {
  ModAdd(out, op, p);
}

template <>
void scl::details::FieldSubtract<Mersenne61>(u64& out, const u64& op) {
  ModSub(out, op, p);
}

template <>
void scl::details::FieldMultiply<Mersenne61>(u64& out, const u64& op) {
  u128 z = (u128)out * op;
  u64 a = z >> 61;
  u64 b = (u64)z;

  a |= b >> 61;
  b &= p;

  ModAdd(a, b, p);
  out = a;
}

template <>
void scl::details::FieldNegate<Mersenne61>(u64& out) {
  ModNeg(out, p);
}

template <>
void scl::details::FieldInvert<Mersenne61>(u64& out) {
  ModInv<u64, std::int64_t>(out, out, p);
}

template <>
bool scl::details::FieldEqual<Mersenne61>(const u64& in1, const u64& in2) {
  return in1 == in2;
}

template <>
void scl::details::FieldFromBytes<Mersenne61>(u64& dest,
                                              const unsigned char* src) {
  dest = *(const u64*)src;
  dest = dest % p;
}

template <>
void scl::details::FieldToBytes<Mersenne61>(unsigned char* dest,
                                            const u64& src) {
  std::memcpy(dest, &src, sizeof(u64));
}

template <>
std::string scl::details::FieldToString<Mersenne61>(const u64& in) {
  return ToHexString(in);
}

template <>
void scl::details::FieldFromString<Mersenne61>(u64& out,
                                               const std::string& src) {
  out = FromHexString<u64>(src);
  out = out % p;
}
