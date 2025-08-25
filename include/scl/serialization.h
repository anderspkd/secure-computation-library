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

#ifndef SCL_SERIALIZATION_H
#define SCL_SERIALIZATION_H

#include <cstdint>
#include <cstring>
#include <type_traits>
#include <vector>

namespace scl {

template <typename T>
struct Serializer;

template <typename T>
concept Serializable =
    requires(T v, const unsigned char* in, unsigned char* out) {
      { Serializer<T>::sizeOf(v) } -> std::same_as<std::size_t>;
      { Serializer<T>::write(v, out) } -> std::same_as<std::size_t>;
      { Serializer<T>::read(v, in) } -> std::same_as<std::size_t>;
    };

template <typename T>
  requires(std::is_trivially_copyable_v<T>)
struct Serializer<T> {
  static constexpr std::size_t sizeOf(const T& /* ignored */) {
    return sizeof(T);
  }

  static constexpr std::size_t write(const T& obj, unsigned char* out) {
    std::memcpy(out, &obj, sizeof(T));
    return sizeOf(obj);
  }

  static constexpr std::size_t read(T& obj, const unsigned char* in) {
    std::memcpy(&obj, in, sizeof(T));
    return sizeOf(obj);
  }
};

template <typename T>
struct Serializer<std::vector<T>> {
  using VecSizeType = std::uint32_t;

  static constexpr std::size_t sizeOf(const std::vector<T>& vec) {
    auto total_size = sizeof(VecSizeType);
    for (const auto& v : vec) {
      total_size += Serializer<T>::sizeOf(v);
    }
    return total_size;
  }

  static constexpr std::size_t write(const std::vector<T>& vec,
                                     unsigned char* out) {
    const auto vec_size = static_cast<VecSizeType>(vec.size());
    auto written = Serializer<VecSizeType>::write(vec_size, out);
    for (const auto& v : vec) {
      written += Serializer<T>::write(v, out + written);
    }
    return written;
  }

  static constexpr std::size_t read(std::vector<T>& vec,
                                    const unsigned char* in) {
    VecSizeType vec_size = 0;
    auto read = Serializer<VecSizeType>::read(vec_size, in);
    vec.resize(vec_size);
    for (std::size_t i = 0; i < vec_size; i++) {
      T v;
      read += Serializer<T>::read(v, in + read);
      vec[i] = std::move(v);
    }
    return read;
  }
};

}  // namespace scl

#endif  // SCL_SERIALIZATION_H
