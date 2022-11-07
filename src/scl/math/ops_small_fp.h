/**
 * @file ops_small_fp.h
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

#ifndef SCL_MATH_OPS_SMALL_FP_H
#define SCL_MATH_OPS_SMALL_FP_H

#include <stdexcept>

namespace scl {
namespace details {

/**
 * @brief Compute a modular addition on two simple types.
 */
template <typename T>
void ModAdd(T& t, const T& v, const T& m) {
  t = t + v;
  if (t >= m) {
    t = t - m;
  }
}

/**
 * @brief Compute a modular subtraction on two simple types.
 */
template <typename T>
void ModSub(T& t, const T& v, const T& m) {
  if (v > t) {
    t = t + m - v;
  } else {
    t = t - v;
  }
}

/**
 * @brief Compute the additive inverse of a simple type.
 */
template <typename T>
void ModNeg(T& t, const T& m) {
  if (t) {
    t = m - t;
  }
}

/**
 * @brief Compute a modular inverse of a simple type.
 */
template <typename T, typename S>
void ModInv(T& t, const T& v, const T& m) {
#define SCL_PARALLEL_ASSIGN(v1, v2, q) \
  do {                                 \
    const auto __temp = v2;            \
    (v2) = (v1) - (q)*__temp;          \
    (v1) = __temp;                     \
  } while (0)

  if (v == 0) {
    throw std::logic_error("0 not invertible modulo prime");
  }

  S k = 0;
  S new_k = 1;
  S r = m;
  S new_r = v;

  while (new_r != 0) {
    const auto q = r / new_r;
    SCL_PARALLEL_ASSIGN(k, new_k, q);
    SCL_PARALLEL_ASSIGN(r, new_r, q);
  }

#undef SCL_PARALLEL_ASSIGN

  if (k < 0) {
    k = k + m;
  }

  t = static_cast<T>(k);
}

}  // namespace details
}  // namespace scl

#endif  // SCL_MATH_OPS_SMALL_FP_H
