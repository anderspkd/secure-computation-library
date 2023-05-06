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
 * The job of Serializer is to provide functionality that reads, writes and
 * determines the size of various objects, primarily for the purpose of writing
 * data to a net::Channel.
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
  static void Write(const T& obj, unsigned char* buf) {
    std::memcpy(buf, &obj, sizeof(T));
  }

  /**
   * @brief Read an obj from a buffer.
   * @param obj where to place the object after reading.
   * @param buf the buffer to read from.
   * @return the number of bytes read. Equal to <code>SizeOf(T)</code>.
   */
  static std::size_t Read(T& obj, const unsigned char* buf) {
    std::memcpy(&obj, buf, sizeof(T));
    return SizeOf(obj);
  }
};

/**
 * @brief Serializer specialization for one dimensional <code>std::vector</code>
 * types.
 *
 * <p>This Serializer will write or read something of type
 * <code>std::vector<T></code> where <code>T</code> is not itself an
 * <code>std::vector</code>. The reason for the latter restriction is to get a
 * reasonable guarantee (that might not hold for all types), that all objects in
 * the vector have the same size.
 *
 * <p>A vector is written as <code>size | content ...</code>.
 */
template <typename T>
struct Serializer<std::vector<T>,
                  std::enable_if_t<!util::IsStdVector<T>::value>> {
  /**
   * @brief Helper. Size of a <code>std::vector</code> size type.
   */
  constexpr static auto SIZE_SIZE = sizeof(typename std::vector<T>::size_type);

  /**
   * @brief Determine the byte size of a vector.
   * @param vec the vector.
   * @return the size of \p vec when written using this Serializer.
   *
   * All elements in the vector are assumed to have the same size, so this
   * function essentially returns <code>4 + sizeof(vec[0]) * vec.size()</code>.
   */
  static std::size_t SizeOf(const std::vector<T>& vec) {
    auto tsz = Serializer<T>::SizeOf(vec[0]);
    return SIZE_SIZE + tsz * vec.size();
  }

  /**
   * @brief Write a vector to a buffer.
   * @param vec the vector.
   * @param buf the buffer where \p vec is written to.
   */
  static void Write(const std::vector<T>& vec, unsigned char* buf) {
    auto tsz = Serializer<T>::SizeOf(vec[0]);
    Serializer<typename std::vector<T>::size_type>::Write(vec.size(), buf);
    auto offset = SIZE_SIZE;
    for (const auto& v : vec) {
      Serializer<T>::Write(v, buf + offset);
      offset += tsz;
    }
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
    Serializer<typename std::vector<T>::size_type>::Read(size, buf);
    vec.reserve(size);
    auto offset = SIZE_SIZE;
    for (std::size_t i = 0; i < size; ++i) {
      T v;
      Serializer<T>::Read(v, buf + offset);
      offset += Serializer<T>::SizeOf(v);
      vec.emplace_back(v);
    }
    return offset;
  }
};

}  // namespace scl::seri

#endif  // SCL_SERIALIZATION_SERIALIZER_H
