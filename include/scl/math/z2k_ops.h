/* SCL --- Secure Computation Library
 * Copyright (C) 2023 Anders Dalskov
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

#ifndef SCL_MATH_Z2K_OPS_H
#define SCL_MATH_Z2K_OPS_H

#include <cstring>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "scl/util/str.h"

namespace scl::math {

/**
 * @brief Add two values modulo a power of 2 without normalization.
 */
template <typename T>
void Z2kAdd(T& dst, const T& op) {
  dst += op;
}

/**
 * @brief Subtract two values modulo a power of 2 without normalization.
 */
template <typename T>
void Z2kSubtract(T& dst, const T& op) {
  dst -= op;
}

/**
 * @brief Multiply two values modulo a power of 2 without normalization.
 */
template <typename T>
void Z2kMultiply(T& dst, const T& op) {
  dst *= op;
}

/**
 * @brief Negate a value modulo a power of 2 without normalization.
 */
template <typename T>
void Z2kNegate(T& v) {
  v = -v;
}

/**
 * @brief Get the least significant bit of a value.
 */
template <typename T>
unsigned Z2kLsb(T& v) {
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
void Z2kInvert(T& v) {
  if (!Z2kLsb(v)) {
    throw std::invalid_argument("value not invertible modulo 2^K");
  }

  std::size_t bits = 5;
  T z = ((v * 3) ^ 2);
  while (bits <= K) {
    z *= 2 - v * z;
    bits *= 2;
  }

  v = z;
}

#define SCL_MASK(T, K) ((static_cast<T>(1) << (K)) - 1)

/**
 * @brief Compute equality modulo a power of 2.
 */
template <typename T, std::size_t K, std::enable_if_t<(K <= 128), bool> = true>
bool Z2kEqual(const T& a, const T& b) {
  return (a & SCL_MASK(T, K)) == (b & SCL_MASK(T, K));
}

/**
 * @brief Read a value from a buffer and truncate it to a power of 2.
 */
template <typename T, std::size_t K, std::enable_if_t<(K <= 128), bool> = true>
void Z2kFromBytes(T& v, const unsigned char* src) {
  v = *(const T*)src;
  v &= SCL_MASK(T, K);
}

/**
 * @brief Write a value modulo a power of 2 to a buffer.
 */
template <typename T, std::size_t K, std::enable_if_t<(K <= 128), bool> = true>
void Z2kToBytes(const T& v, unsigned char* dest) {
  // normalization is deferred until elements are written somewhere, so we v
  // needs to be normalized before we can write it.
  auto w = v & SCL_MASK(T, K);
  std::memcpy(dest, (unsigned char*)&w, (K - 1) / 8 + 1);
}

/**
 * @brief Read a value modulo a power of 2 from a string
 * @param v where to store the value read
 * @param str the string
 */
template <typename T, std::size_t K, std::enable_if_t<(K <= 128), bool> = true>
void Z2kFromString(T& v, const std::string& str) {
  v = util::FromHexString<T>(str);
  v &= SCL_MASK(T, K);
}

/**
 * @brief Convert a value modolu a power of 2 to a string.
 * @param v the value
 */
template <typename T, std::size_t K, std::enable_if_t<(K <= 128), bool> = true>
std::string Z2kToString(const T& v) {
  auto w = v & SCL_MASK(T, K);
  return util::ToHexString(w);
}

}  // namespace scl::math

#endif  // SCL_MATH_Z2K_OPS_H
