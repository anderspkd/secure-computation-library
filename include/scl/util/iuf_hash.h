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

#ifndef SCL_UTIL_IUF_HASH_H
#define SCL_UTIL_IUF_HASH_H

#include <array>
#include <cstdint>
#include <iomanip>
#include <memory>
#include <string>
#include <vector>

#include "scl/serialization/serializer.h"

namespace scl::util {

/**
 * @brief IUF (Init-Update-Finalize) interface for hash functions.
 * @tparam HASH hash implementation.
 *
 * IUFHash provides a CRTP style interface for a hash function implementation
 *
 * @see Sha3
 * @see Sha256
 */
template <typename HASH>
struct IUFHash {
  /**
   * @brief Update the hash function with a set of bytes.
   * @param bytes a pointer to a number of bytes.
   * @param n the number of bytes.
   * @return the updated Hash object.
   */
  IUFHash<HASH>& update(const unsigned char* bytes, std::size_t n) {
    static_cast<HASH*>(this)->hash(bytes, n);
    return *this;
  };

  /**
   * @brief Update the hash function with the content from a byte vector.
   * @param data a vector of bytes.
   * @return the updated Hash object.
   */
  IUFHash<HASH>& update(const std::vector<unsigned char>& data) {
    return update(data.data(), data.size());
  };

  /**
   * @brief Update the hash function with the content from a byte STL array.
   * @param data the array
   * @return the updated Hash object.
   */
  template <std::size_t N>
  IUFHash<HASH>& update(const std::array<unsigned char, N>& data) {
    return update(data.data(), N);
  }

  /**
   * @brief Update the hash function with the content of a string.
   * @param string the string.
   * @return the updated Hash object.
   */
  IUFHash<HASH>& update(std::string_view string) {
    return update(reinterpret_cast<const unsigned char*>(string.data()),
                  string.size());
  }

  /**
   * @brief Update the hash function with the content of a serializable type.
   * @param data the data.
   * @return the updated Hash object.
   */
  template <typename T>
  IUFHash<HASH>& update(const T& data) {
    using Sr = seri::Serializer<T>;
    const auto size = Sr::sizeOf(data);
    const auto buf = std::make_unique<unsigned char[]>(size);
    Sr::write(data, buf.get());
    return update(buf.get(), size);
  }

  /**
   * @brief Finalize and return the digest.
   * @return a digest.
   */
  auto finalize() {
    auto digest = static_cast<HASH*>(this)->write();
    return digest;
  };
};

}  // namespace scl::util

#endif  // SCL_UTIL_IUF_HASH_H
