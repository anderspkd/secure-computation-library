/* SCL --- Secure Computation Library
 * Copyright (C) 2024 Anders Dalskov
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

#ifndef SCL_SERIALIZATION_SERIALIZABLE_H
#define SCL_SERIALIZATION_SERIALIZABLE_H

#include "scl/serialization/serializer.h"

namespace scl::seri {

/**
 * @brief Requirements for a type to be serializable in SCL.
 */
template <typename T>
concept Serializable =
    requires(T v, const unsigned char* in, unsigned char* out) {
      { Serializer<T>::sizeOf(v) } -> std::same_as<std::size_t>;
      { Serializer<T>::write(v, out) } -> std::same_as<std::size_t>;
      { Serializer<T>::read(v, in) } -> std::same_as<std::size_t>;
    };

}  // namespace scl::seri

#endif  // SCL_SERIALIZATION_SERIALIZABLE_H
