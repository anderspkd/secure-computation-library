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

#ifndef SCL_SERIALIZATION_MATH_SERIALIZERS_H
#define SCL_SERIALIZATION_MATH_SERIALIZERS_H

#include "scl/math/ff.h"
#include "scl/math/mat.h"
#include "scl/math/vec.h"
#include "scl/serialization/serializer.h"

namespace scl::seri {

/**
 * @brief Serializer specialization for math::FF types.
 */
template <typename T>
struct Serializer<math::FF<T>> {
  /**
   * @brief Determine the size of an math::FF value.
   * @param ignored an element, which is ignored.
   *
   * The size of an math::FF element can be determined from its type alone, so
   * the argument is ignored.
   */
  static constexpr std::size_t SizeOf(const math::FF<T>& ignored) {
    (void)ignored;
    return math::FF<T>::ByteSize();
  }

  /**
   * @brief Write an math::FF element to a buffer.
   * @param elem the element.
   * @param buf the buffer.
   *
   * Calls math::FF::Write.
   */
  static void Write(const math::FF<T>& elem, unsigned char* buf) {
    elem.Write(buf);
  }

  /**
   * @brief Read an math::FF element from a buffer.
   * @param elem output variable holding the read element after reading.
   * @param buf the buffer.
   * @return the number of bytes read.
   *
   * Calls math::FF::Read() and returns math::FF::ByteSize();
   */
  static std::size_t Read(math::FF<T>& elem, const unsigned char* buf) {
    elem = math::FF<T>::Read(buf);
    return math::FF<T>::ByteSize();
  }
};

/**
 * @brief Serializer specialization for math::Vec.
 */
template <typename T>
struct Serializer<math::Vec<T>> {
  /**
   * @brief Helper. Size of a math::Vec size type.
   */
  static constexpr auto SIZE_TYPE_SIZE =
      sizeof(typename math::Vec<T>::SizeType);

  /**
   * @brief Size of a vector.
   * @param vec the vector.
   */
  static std::size_t SizeOf(const math::Vec<T>& vec) {
    return vec.ByteSize() + SIZE_TYPE_SIZE;
  }

  /**
   * @brief Write a math::Vec to a buffer.
   * @param vec the vector.
   * @param buf the buffer.
   */
  static void Write(const math::Vec<T>& vec, unsigned char* buf) {
    const auto sz = vec.Size();
    std::memcpy(buf, &sz, SIZE_TYPE_SIZE);
    vec.Write(buf + SIZE_TYPE_SIZE);
  }

  /**
   * @brief Read a math::Vec from a buf.
   * @param vec the vector.
   * @param buf the buffer.
   * @return the number of bytes read.
   */
  static std::size_t Read(math::Vec<T>& vec, const unsigned char* buf) {
    typename math::Vec<T>::SizeType sz;
    std::memcpy(&sz, buf, SIZE_TYPE_SIZE);
    vec = math::Vec<T>::Read(sz, buf + SIZE_TYPE_SIZE);
    return SizeOf(vec);
  }
};

/**
 * @brief Serializer specialization for a math::Mat.
 */
template <typename T>
struct Serializer<math::Mat<T>> {
  /**
   * @brief Helper. Size of the type used to denote matrix dimensions.
   */
  static constexpr auto SIZE_TYPE_SIZE =
      sizeof(typename math::Mat<T>::SizeType);

  /**
   * @brief Size of a matrix.
   * @param mat the matrix.
   *
   * The size of a matrix is the determined as the size of the content plus two
   * times <code>SIZE_TYPE_SIZE</code>.
   */
  static std::size_t SizeOf(const math::Mat<T>& mat) {
    return mat.ByteSize() + 2 * SIZE_TYPE_SIZE;
  }

  /**
   * @brief Write a matrix to a buffer.
   * @param mat the matrix.
   * @param buf the buffer.
   */
  static void Write(const math::Mat<T>& mat, unsigned char* buf) {
    const auto c = mat.Cols();
    std::memcpy(buf, &c, SIZE_TYPE_SIZE);
    const auto r = mat.Rows();
    std::memcpy(buf + SIZE_TYPE_SIZE, &r, SIZE_TYPE_SIZE);
    mat.Write(buf + 2 * SIZE_TYPE_SIZE);
  }

  /**
   * @brief Read a matrix from a buffer.
   * @param mat where to store the matrix after reading.
   * @param buf the buffer.
   * @return the number of bytes read.
   */
  static std::size_t Read(math::Mat<T>& mat, const unsigned char* buf) {
    typename math::Mat<T>::SizeType r;
    typename math::Mat<T>::SizeType c;
    std::memcpy(&c, buf, SIZE_TYPE_SIZE);
    std::memcpy(&r, buf + SIZE_TYPE_SIZE, SIZE_TYPE_SIZE);
    mat = math::Mat<T>::Read(r, c, buf + 2 * SIZE_TYPE_SIZE);
    return SizeOf(mat);
  }
};

}  // namespace scl::seri

#endif  // SCL_SERIALIZATION_MATH_SERIALIZERS_H
