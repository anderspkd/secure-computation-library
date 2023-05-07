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

#ifndef SCL_SERIALIZATION_SERIALIZER_H
#define SCL_SERIALIZATION_SERIALIZER_H

#include <cstddef>
#include <cstring>
#include <type_traits>
#include <vector>

#include "scl/util/traits.h"

namespace scl::seri {

/**
 * @brief Serializer.
 *
 * <p>A Serializer is meant to provide functionality for writing to, and reading
 * from, a binary format, and is used for example when sending data across a
 * network. To make the type \p T serializable, a specialization of Serializer
 * must be provided which defines the following three methods:
 *
 * <ul>
 *
 * <li><code>static std::size_t SizeOf(const T& obj)</code>. This function
 * should return the <i>binary</i> size of <code>obj</code>. More precisely, it
 * should return the size of the memory needed to store an object of type \p
 * T.</li>
 *
 * <li><code>static std::size_t Write(const T& obj, unsigned char* buf)</code>
 * this function should write <code>obj</code> in whatever binary format is
 * appropriate to the buffer <code>buf</code>. The function can assume that
 * <code>buf</code> points to at least <code>SizeOf(obj)</code> bytes of memory.
 * The return value of this function should be the number of bytes written to
 * <code>buf</code>.</li>
 *
 * <li><code>static std::size_t Read(T& obj, const unsigned char* buf)</code>
 * this function should read an object of type \p T from <code>buf</code> and
 * assign it to <code>obj</code>. The return value should be the number of bytes
 * read from <code>buf</code>.</li>
 * </ul>
 */
template <typename T, typename = void>
struct Serializer;

/**
 * @brief Serializer specialization for trivially copyable types.
 *
 * This Serializer reads and writes types that are trivially copyable. In a
 * nutshell, this includes all types that can be constructed via a call to
 * <code>std::memcpy</code>.
 *
 * @see https://en.cppreference.com/w/cpp/named_req/TriviallyCopyable
 */
template <typename T>
struct Serializer<T, std::enable_if_t<std::is_trivially_copyable<T>::value>> {
  /**
   * @brief Determine the size of an object.
   * @param ignored the object, which is ignored.
   *
   * The size of a trivially copyable object is assumed to be decidable from the
   * type itself. This function returns <code>sizeof(T)</code>.
   */
  static constexpr std::size_t SizeOf(const T& ignored) {
    (void)ignored;
    return sizeof(T);
  }

  /**
   * @brief Write an object to a buffer.
   * @param obj the object to write.
   * @param buf the buffer to write the object to.
   */
  static constexpr std::size_t Write(const T& obj, unsigned char* buf) {
    std::memcpy(buf, &obj, sizeof(T));
    return sizeof(T);
  }

  /**
   * @brief Read an obj from a buffer.
   * @param obj where to place the object after reading.
   * @param buf the buffer to read from.
   * @return the number of bytes read. Equal to <code>SizeOf(T)</code>.
   */
  static constexpr std::size_t Read(T& obj, const unsigned char* buf) {
    std::memcpy(&obj, buf, sizeof(T));
    return SizeOf(obj);
  }
};

/**
 * @brief Serializer specialization for one dimensional <code>std::vector</code>
 * types.
 */
template <typename T>
struct Serializer<std::vector<T>> {
 private:
  using VectorSizeType = typename std::vector<T>::size_type;
  constexpr static auto SIZE_SIZE = sizeof(VectorSizeType);

 public:
  /**
   * @brief Determine the byte size of a vector.
   * @param vec the vector.
   * @return the size of \p vec when written using this Serializer.
   */
  static std::size_t SizeOf(const std::vector<T>& vec) {
    auto size = SIZE_SIZE;
    for (const auto& v : vec) {
      size += Serializer<T>::SizeOf(v);
    }
    return size;
  }

  /**
   * @brief Write a vector to a buffer.
   * @param vec the vector.
   * @param buf the buffer where \p vec is written to.
   * @return the number of bytes written to buf.
   */
  static std::size_t Write(const std::vector<T>& vec, unsigned char* buf) {
    Serializer<VectorSizeType>::Write(vec.size(), buf);
    auto offset = SIZE_SIZE;
    for (const auto& v : vec) {
      offset += Serializer<T>::Write(v, buf + offset);
    }
    return offset;
  }

  /**
   * @brief Read a vector from a buffer.
   * @param vec where the vector read is stored.
   * @param buf the buffer.
   * @return the number of bytes read from buf.
   *
   * This function reads a size from \p buf and uses it to <code>reserve</code>
   * space in \p vec. Elements are then read one by one from \p buf.
   */
  static std::size_t Read(std::vector<T>& vec, const unsigned char* buf) {
    typename std::vector<T>::size_type size;
    Serializer<VectorSizeType>::Read(size, buf);
    vec.reserve(size);
    auto offset = SIZE_SIZE;
    for (std::size_t i = 0; i < size; ++i) {
      T v;
      offset += Serializer<T>::Read(v, buf + offset);
      vec.emplace_back(v);
    }
    return offset;
  }
};

}  // namespace scl::seri

#endif  // SCL_SERIALIZATION_SERIALIZER_H
