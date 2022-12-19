/**
 * @file gf7.cc
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

#include "./gf7.h"

#include "scl/math/ff_ops.h"

using GF7 = scl_tests::GaloisField7;

template <>
void scl::details::FieldConvertIn<GF7>(unsigned char& out, int v) {
  auto r = v % 7;
  out = r < 0 ? 7 + r : r;
}

template <>
void scl::details::FieldAdd<GF7>(unsigned char& out, const unsigned char& op) {
  out = (out + op) % 7;
}

template <>
void scl::details::FieldSubtract<GF7>(unsigned char& out,
                                      const unsigned char& op) {
  if (out < op) {
    out = 7 + out - op;
  } else {
    out = out - op;
  }
}

template <>
void scl::details::FieldMultiply<GF7>(unsigned char& out,
                                      const unsigned char& op) {
  out = (out * op) % 7;
}

template <>
void scl::details::FieldNegate<GF7>(unsigned char& out) {
  out = (7 - out) % 7;
}

template <>
void scl::details::FieldInvert<GF7>(unsigned char& out) {
  unsigned char inv;
  switch (out) {
    case 1:
    case 6:
      inv = out;
      break;
    case 2:
      inv = 4;
      break;
    case 3:
      inv = 5;
      break;
    case 4:
      inv = 2;
      break;
    case 5:
      inv = 3;
      break;
    default:
      throw std::logic_error("0 not invertible modulo prime");
  }
  out = inv;
}

template <>
bool scl::details::FieldEqual<GF7>(const unsigned char& in1,
                                   const unsigned char& in2) {
  return in1 == in2;
}

template <>
void scl::details::FieldFromBytes<GF7>(unsigned char& dest,
                                       const unsigned char* src) {
  dest = *src;
  dest = dest % 7;
}

template <>
void scl::details::FieldToBytes<GF7>(unsigned char* dest,
                                     const unsigned char& src) {
  *dest = src;
}

template <>
std::string scl::details::FieldToString<GF7>(const unsigned char& in) {
  std::stringstream ss;
  ss << (int)in;
  return ss.str();
}
