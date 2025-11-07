/* SCL --- Secure Computation Library
 * Copyright (C) 2025 Anders Dalskov
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

#pragma once

#include <cstdint>
#include <cstring>
#include <type_traits>
#include <vector>

namespace scl {

/**
 * @brief Serializer type.
 *
 * A type \p T is serializable if it <code>Serializer<T></code> contains three
 * static functions.
 *
 * @code
 * struct Thing {
 *   static std::size_t sizeOf(Thing t) {
 *      // return the size in bytes of the object t of type Thing
 *   }
 *
 *   static std::size_t write(Thing t, unsigned char* out) {
 *      // write t to out.
 *      // out will be guaranteed to point to at least sizeOf(t) available bytes
 *      // return the amount of bytes written to out
 *   }
 *
 *   static std::size_t read(Thing& t, const unsigned char* in) {
 *      // read a Thing from in, store the result in t
 *      // return the amount of bytes read from in
 *   }
 * }
 * @endcode
 *
 * Note that no guarantees are made about the buffer passed to
 * <code>read</code>.
 *
 * Serializer's mostly play a role in reading and writing to Packets for the
 * purpose of communicating with other parties.
 */
template <typename T>
struct Serializer;

/**
 * @brief Serializable concept.
 */
template <typename T>
concept Serializable =
    requires(T v, T& r, const unsigned char* in, unsigned char* out) {
      { Serializer<T>::sizeOf(v) } -> std::same_as<std::size_t>;
      { Serializer<T>::write(v, out) } -> std::same_as<std::size_t>;
      { Serializer<T>::read(r, in) } -> std::same_as<std::size_t>;
    };

/**
 * @brief Serializer for trivially copyable types.
 */
template <typename T>
  requires(std::is_trivially_copyable_v<T>)
struct Serializer<T> {

  /**
   * @brief Determine the size of a value.
   */
  static constexpr std::size_t sizeOf(const T& /* ignored */) {
    return sizeof(T);
  }

  /**
   * @brief Write the value to a buffer.
   */
  static constexpr std::size_t write(const T& obj, unsigned char* out) {
    std::memcpy(out, &obj, sizeof(T));
    return sizeOf(obj);
  }

  /**
   * @brief Read a value from a buffer.
   */
  static constexpr std::size_t read(T& obj, const unsigned char* in) {
    std::memcpy(&obj, in, sizeof(T));
    return sizeOf(obj);
  }
};

/**
 * @brief Serializer for STL vectors of something serializable.
 */
template <Serializable T>
struct Serializer<std::vector<T>> {

  /**
   * @brief Type used to denote the size of the vector.
   */
  using VecSizeType = std::uint32_t;


  /**
   * @brief Get the bytes required to write \p vec with this serializer.
   */
  static constexpr std::size_t sizeOf(const std::vector<T>& vec) {
    auto total_size = sizeof(VecSizeType);
    for (const auto& v : vec) {
      total_size += Serializer<T>::sizeOf(v);
    }
    return total_size;
  }

  /**
   * @brief Write a vector to a buffer.
   */
  static constexpr std::size_t write(const std::vector<T>& vec,
                                     unsigned char* out) {
    const auto vec_size = static_cast<VecSizeType>(vec.size());
    auto written = Serializer<VecSizeType>::write(vec_size, out);
    for (const auto& v : vec) {
      written += Serializer<T>::write(v, out + written);
    }
    return written;
  }

  /**
   * @brief Read a vector from a buffer.
   */
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
