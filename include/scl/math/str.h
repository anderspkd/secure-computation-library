/**
 * @file str.h
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

#ifndef _SCL_MATH_STR_H
#define _SCL_MATH_STR_H

#include <stdexcept>

#include "scl/math/bases.h"

namespace scl {
namespace details {

namespace {

inline bool InRange(const char c, const int lower, const int upper) {
  return lower <= c && c <= upper;
}

#define _SCL_INVALID(what) \
  throw std::invalid_argument("encountered invalid " what " character");

inline unsigned FromBinary(const char c) {
  if (InRange(c, '0', '1')) return c - '0';
  _SCL_INVALID("binary");
}

inline unsigned FromDecimal(const char c) {
  if (InRange(c, '0', '9')) return c - '0';
  _SCL_INVALID("decimal");
}

inline unsigned FromHex(const char c0, const char c1) {
  unsigned v = 0;
#define _SCL_I(__sym)                  \
  do {                                 \
    if (InRange(__sym, '0', '9'))      \
      v += __sym - '0';                \
    else if (InRange(__sym, 'A', 'F')) \
      v += __sym - 'A' + 10;           \
    else if (InRange(__sym, 'a', 'f')) \
      v += __sym - 'a' + 10;           \
    else                               \
      _SCL_INVALID("hex");             \
  } while (0)
  _SCL_I(c0);
  v = v << 4;
  _SCL_I(c1);
#undef _SCL_I
  return v;
}

inline unsigned FromBase64(const char c) {
  unsigned v = 0;
  if (InRange(c, 'A', 'Z'))
    v += c - 'A';
  else if (InRange(c, 'a', 'z'))
    v += c - 'a' + 26;
  else if (InRange(c, '0', '9'))
    v += c - '0' + 52;
  else if (c == '+')
    v += 62;
  else if (c == '/')
    v += 63;
  else
    _SCL_INVALID("base64");
  return v;
}

#undef _SCL_INVALID

}  // namespace

/**
 * @brief Convert a string in binary.
 *
 * If the input is a string of 1s and 0s then the output will be the value
 * obtained by intepreting the string as a big-endian integer in binary.
 *
 * @tparam T the output type
 */
template <typename T>
void FromStringBinary(T& t, const std::string& s) {
  t = 0;
  for (const auto b : s) {
    t *= 2;
    t += FromBinary(b);
  }
}

/**
 * @brief Convert a string in decimal.
 * @tparam T the output type
 */
template <typename T>
void FromStringDecimal(T& t, const std::string& s) {
  t = 0;
  for (const auto d : s) {
    t *= 10;
    t += FromDecimal(d);
  }
}

/**
 * @brief Convert a string in hex.
 *
 * Normal conventions for hex strings apply, although the <code>0x</code> prefix
 * is not permitted. The input is assumed to encode an integer in big endian.
 *
 * @tparam T the output type
 */
template <typename T>
void FromStringHex(T& t, const std::string& s) {
  auto n = s.size();
  if (n % 2) throw std::invalid_argument("odd-length hex string");
  t = 0;
  for (std::size_t i = 0; i < n; i += 2) {
    char c0 = s[i];
    char c1 = s[i + 1];
    t *= 256;
    t += FromHex(c0, c1);
  }
}

/**
 * @brief Convert a string in base64.
 *
 * The input is treated as an encoding of a big-endian value.
 *
 * @tparam T the output type
 */
template <typename T>
void FromStringBase64(T& t, const std::string& s) {
  auto n = s.size();
  if (n % 4) {
    throw std::invalid_argument("invalid length base64 string");
  }

  t = 0;
  std::size_t i = 0;
  bool has_padding = s[n - 1] == '=';
  auto m = n - (has_padding ? 4 : 0);

  while (i < m) {
    unsigned c0 = FromBase64(s[i++]);
    unsigned c1 = FromBase64(s[i++]);
    unsigned c2 = FromBase64(s[i++]);
    unsigned c3 = FromBase64(s[i++]);
    t = t << 8;
    t += c0 << 2 | c1 >> 4;
    t = t << 8;
    t += (c1 & 0x0F) << 4 | c2 >> 2;
    t = t << 8;
    t += (c2 & 0x03) << 6 | c3;
  }

  // there's at least one padding character at the very end here.
  if (has_padding) {
    char c0 = s[i++];
    char c1 = s[i++];
    char c2 = s[i++];
    if (c0 == '=' || c1 == '=')
      throw std::invalid_argument("invalid base64 padding");
    t = t << 8;
    unsigned b1 = FromBase64(c1);
    t += FromBase64(c0) << 2 | b1 >> 4;
    if (c2 != '=') {
      t = t << 8;
      t += (b1 & 0x0F) << 4 | FromBase64(c2) >> 2;
    }
  }
}

/**
 * @brief Convert a string from some base to a simple type.
 */
template <typename T>
void FromStringSimpleType(T& t, const std::string& s,
                          enum scl::NumberBase base) {
  if (base == NumberBase::BINARY)
    FromStringBinary(t, s);
  else if (base == NumberBase::DECIMAL)
    FromStringDecimal(t, s);
  else if (base == NumberBase::HEX)
    FromStringHex(t, s);
  else  // if (base == NumberBase::BASE64)
    FromStringBase64(t, s);
}

/**
 * @brief Convert value into a string.
 */
template <typename T>
std::string ToString(const T& v);

}  // namespace details
}  // namespace scl

#endif  // _SCL_MATH_STR_H
