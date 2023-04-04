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

namespace scl::util {

/**
 * @brief Helper type to determine if a type has Read/Write functionality.
 * @tparam T a type which may have read/write functionality.
 *
 * <p>This trait checks the type \p T to see if it has two functions with a
 * particular signature. The functions we are looking for are:
 *
 * <ul>
 * <li><code>T Read(const unsigned char*)</code></li>
 * <li><code>void Write(unsigned char*)</code></li>
 * </ul>
 *
 * <p>Where <code>Read</code> must be a static method, and <code>Write</code> a
 * member function.
 *
 * <p>Types satisfying this trait are math::FF and math::Z2k.
 */
template <typename T, typename, typename>
struct Serializable;

/// @cond

template <typename, typename = void, typename = void>
struct Serializable : std::false_type {};

template <typename T>
struct Serializable<
    T,
    std::void_t<decltype(T::Read(std::declval<const unsigned char*>()))>,
    std::void_t<decltype(std::declval<T>().Write(
        std::declval<unsigned char*>()))>> : std::true_type {};

/// @endcond

}  // namespace scl::util

#endif  // SCL_UTIL_TRAITS_H
