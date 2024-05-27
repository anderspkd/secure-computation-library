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

#ifndef SCL_SERIALIZATION_SERIALIZER_H
#define SCL_SERIALIZATION_SERIALIZER_H

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <vector>

namespace scl::seri {

/**
 * @brief Serializer type.
 *
 * <p>Serializer's are used throughout SCL whenever data has to be converted
 * to/from a binary format. Deciding how this conversion is done is Serializer's
 * job.
 *
 * <p>SCL contains Serializer implementations for most of its types, as well as
 * couple of a "standard" types (such as trivially copyable objects). Adding a
 * new Serializer is easy, and can be done by providing a specialization of the
 * Serializer type, which must include a couple of methods. An example is shown
 * below:
 *
 * @code
 * template <>
 * struct scl::seri::Serializer<MyType> {
 *   // should return the binary size of an object
 *   static std::size_t sizeOf(const MyType& obj) {
 *     return binarySize(obj);
 *   }
 *
 *   // writes the object to a buffer. The buffer can be assumed to have space
 *   // for at least sizeOf(obj) bytes. The function must return the number of
 *   // bytes written to buf.
 *   static std::size_t write(const MyType& obj, unsigned char* buf) {
 *     writeMyTypeToBuf(obj, buf);
 *     return sizeOf(obj);
 *   }
 *
 *   // reads an object from a buffer and assigns it to the first argument.
 *   // The return value must be the number of bytes read from buf.
 *   static std::size_t read(MyType& obj, const unsigned char* buf) {
 *     obj = readMyTypeFromBuf(buf);
 *     return sizeOf(obj);
 *   }
 * };
 * @endcode
 *
 * @see Serializable.
 */
template <typename T, typename = void>
struct Serializer;

/**
 * @brief Serializer specialization for trivially copyable types.
 * @see https://en.cppreference.com/w/cpp/named_req/TriviallyCopyable
 */
template <typename T>
struct Serializer<T, std::enable_if_t<std::is_trivially_copyable<T>::value>> {
  /**
   * @brief Determine the size of an object.
   * @return the size of something of the trivially copyable type.
   */
  static constexpr std::size_t sizeOf(const T& /* ignored */) {
    return sizeof(T);
  }

  /**
   * @brief Write an object to a buffer.
   * @param obj the object to write.
   * @param buf the buffer to write the object to.
   */
  static constexpr std::size_t write(const T& obj, unsigned char* buf) {
    std::memcpy(buf, &obj, sizeof(T));
    return sizeOf(obj);
  }

  /**
   * @brief Read an obj from a buffer.
   * @param obj where to place the object after reading.
   * @param buf the buffer to read from.
   * @return the number of bytes read. Equal to <code>SizeOf(T)</code>.
   */
  static constexpr std::size_t read(T& obj, const unsigned char* buf) {
    std::memcpy(&obj, buf, sizeof(T));
    return sizeOf(obj);
  }
};

/**
 * @brief Type used to carry information about the size of an STL vector.
 */
using StlVecSizeType = std::uint32_t;

/**
 * @brief Serializer specialization for STL vector of bytes.
 */
template <>
struct Serializer<std::vector<unsigned char>> {
 public:
  /**
   * @brief Determine the size of the byte vector.
   * @param data the vector.
   * @return the size of \p data in bytes.
   */
  static std::size_t sizeOf(const std::vector<unsigned char>& data) {
    return Serializer<StlVecSizeType>::sizeOf(data.size()) + data.size();
  }

  /**
   * @brief Write a byte vector to a buffer.
   * @param data the vector.
   * @param buf the buffer.
   * @return the number of bytes written to \p buf.
   */
  static std::size_t write(const std::vector<unsigned char>& data,
                           unsigned char* buf) {
    const auto offset = Serializer<StlVecSizeType>::write(data.size(), buf);
    std::memcpy(buf + offset, data.data(), data.size());
    return sizeOf(data);
  }

  /**
   * @brief Read a byte vector from a buffer.
   * @param data where to store the byte vector read.
   * @param buf the buffer to read from.
   * @return the number of bytes read.
   */
  static std::size_t read(std::vector<unsigned char>& data,
                          const unsigned char* buf) {
    StlVecSizeType size = 0;
    const auto offset = Serializer<StlVecSizeType>::read(size, buf);
    data.resize(size);
    std::memcpy(data.data(), buf + offset, size);
    return sizeOf(data);
  }
};

/**
 * @brief Serializer specialization for generic <code>std::vector</code> types.
 */
template <typename T>
struct Serializer<std::vector<T>> {
 public:
  /**
   * @brief Determine the byte size of a vector.
   * @param vec the vector.
   * @return the size of \p vec when written using this Serializer.
   */
  static std::size_t sizeOf(const std::vector<T>& vec) {
    auto size = Serializer<StlVecSizeType>::sizeOf(vec.size());
    for (const auto& v : vec) {
      size += Serializer<T>::sizeOf(v);
    }
    return size;
  }

  /**
   * @brief Write a vector to a buffer.
   * @param vec the vector.
   * @param buf the buffer where \p vec is written to.
   * @return the number of bytes written to buf.
   */
  static std::size_t write(const std::vector<T>& vec, unsigned char* buf) {
    auto offset = Serializer<StlVecSizeType>::write(vec.size(), buf);
    for (const auto& v : vec) {
      offset += Serializer<T>::write(v, buf + offset);
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
  static std::size_t read(std::vector<T>& vec, const unsigned char* buf) {
    StlVecSizeType size = 0;
    auto offset = Serializer<StlVecSizeType>::read(size, buf);
    vec.resize(size);
    for (std::size_t i = 0; i < size; ++i) {
      T v;
      offset += Serializer<T>::read(v, buf + offset);
      vec[i] = std::move(v);
    }
    return offset;
  }
};

}  // namespace scl::seri

#endif  // SCL_SERIALIZATION_SERIALIZER_H
