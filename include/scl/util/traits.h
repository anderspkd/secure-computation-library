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

#ifndef SCL_UTIL_TRAITS_H
#define SCL_UTIL_TRAITS_H

#include <type_traits>
#include <utility>
#include <vector>

namespace scl::util {

/// @cond

template <typename>
struct IsStdVectorImpl : std::false_type {};

template <typename T, typename A>
struct IsStdVectorImpl<std::vector<T, A>> : std::true_type {};

// https://stackoverflow.com/a/35207812
template <typename T, typename V>
struct HasOperatorMulImpl {
  template <typename TT, typename VV>
  static auto Test(TT*) -> decltype(std::declval<TT>() * std::declval<VV>());

  template <typename, typename>
  static auto Test(...) -> std::false_type;

  using Type = typename std::is_same<T, decltype(Test<T, V>(0))>::type;
};

/// @endcond

/**
 * @brief Trait for determining if two types can be multipled.
 * @tparam T the first type.
 * @tparam V the second type.
 *
 * This trait evalutes to an std::true_type if <code>T operator*(V)</code> is
 * defined.
 */
template <typename T, typename V>
struct HasOperatorMul : HasOperatorMulImpl<T, V>::Type {};

}  // namespace scl::util

#endif  // SCL_UTIL_TRAITS_H
