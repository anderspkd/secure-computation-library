/**
 * @file details.h
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

#ifndef _SCL_MATH_Z2K_DETAILS_H
#define _SCL_MATH_Z2K_DETAILS_H

#include <cstring>
#include <string>
#include <type_traits>

#include "scl/math/str.h"

namespace scl {
namespace details {

/**
 * @brief Add two values modulo a power of 2 without normalization.
 */
template <typename T>
void AddZ2k(T& dst, const T& op) {
  dst += op;
}

/**
 * @brief Subtract two values modulo a power of 2 without normalization.
 */
template <typename T>
void SubtractZ2k(T& dst, const T& op) {
  dst -= op;
}

/**
 * @brief Multiply two values modulo a power of 2 without normalization.
 */
template <typename T>
void MultiplyZ2k(T& dst, const T& op) {
  dst *= op;
}

/**
 * @brief Negate a value modulo a power of 2 without normalization.
 */
template <typename T>
void NegateZ2k(T& v) {
  v = -v;
}

/**
 * @brief Get the least significant bit of a value.
 */
template <typename T>
unsigned LsbZ2k(T& v) {
  return v & 1;
}

/**
 * @brief Compute the inverse of a number modulo a power of 2.
 *
 * This function only works on bit-lengths smaller-or-equal than 128. In theses
 * cases, the template type T is guaranteed to be either std::uint64_t or
 * __uint128_t.
 *
 * @param v the value to invert
 */
template <typename T, std::size_t K, std::enable_if_t<(K <= 128), bool> = true>
void InvertZ2k(T& v) {
  if (!LsbZ2k(v))
    throw std::invalid_argument("value not invertible modulo 2^K");

  std::size_t bits = 5;
  T z = ((v * 3) ^ 2);
  while (bits <= K) {
    z *= 2 - v * z;
    bits *= 2;
  }

  v = z;
}

#define _SCL_MASK(T, K) ((static_cast<T>(1) << K) - 1)

/**
 * @brief Compute equality modulo a power of 2.
 */
template <typename T, std::size_t K, std::enable_if_t<(K <= 128), bool> = true>
bool EqualZ2k(const T& a, const T& b) {
  return (a & _SCL_MASK(T, K)) == (b & _SCL_MASK(T, K));
}

/**
 * @brief Read a value from a buffer and truncate it to a power of 2.
 */
template <typename T, std::size_t K, std::enable_if_t<(K <= 128), bool> = true>
void ReadZ2k(T& v, const unsigned char* src) {
  v = *(const T*)src;
  v &= _SCL_MASK(T, K);
}

/**
 * @brief Write a value modulo a power of 2 to a buffer.
 */
template <typename T, std::size_t K, std::enable_if_t<(K <= 128), bool> = true>
void WriteZ2k(const T& v, unsigned char* dest) {
  // normalization is deferred until elements are written somewhere, so we v
  // needs to be normalized before we can write it.
  auto w = v & _SCL_MASK(T, K);
  std::memcpy(dest, (unsigned char*)&w, (K - 1) / 8 + 1);
}

/**
 * @brief Read a value modulo a power of 2 from a string
 * @param v where to store the value read
 * @param str the string
 * @param base the base of the string
 */
template <typename T, std::size_t K, std::enable_if_t<(K <= 128), bool> = true>
void FromStringZ2k(T& v, const std::string& str, enum scl::NumberBase base) {
  FromStringSimpleType(v, str, base);
  v &= _SCL_MASK(T, K);
}

/**
 * @brief Convert a value modolu a power of 2 to a string.
 * @param v the value
 */
template <typename T, std::size_t K, std::enable_if_t<(K <= 128), bool> = true>
std::string ToStringZ2k(const T& v) {
  auto w = v & _SCL_MASK(T, K);
  return ToString(w);
}

}  // namespace details
}  // namespace scl

#endif  // _SCL_MATH_Z2K_DETAILS_H
