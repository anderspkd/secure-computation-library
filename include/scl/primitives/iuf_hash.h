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

#ifndef SCL_PRIMITIVES_IUF_HASH_H
#define SCL_PRIMITIVES_IUF_HASH_H

#include <array>
#include <memory>
#include <vector>

#include "scl/serialization.h"

namespace scl {

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
   */
  IUFHash<HASH>& update(const unsigned char* bytes, std::size_t n) {
    static_cast<HASH*>(this)->hash(bytes, n);
    return *this;
  };

  /**
   * @brief Update the hash function with the content from a byte vector.
   */
  IUFHash<HASH>& update(const std::vector<unsigned char>& data) {
    return update(data.data(), data.size());
  };

  /**
   * @brief Update the hash function with the content from a byte STL array.
   */
  template <std::size_t N>
  IUFHash<HASH>& update(const std::array<unsigned char, N>& data) {
    return update(data.data(), N);
  }

  /**
   * @brief Update the hash function with the content of a string.
   */
  IUFHash<HASH>& update(std::string_view string) {
    return update(reinterpret_cast<const unsigned char*>(string.data()),
                  string.size());
  }

  /**
   * @brief Update the hash function with the content of a serializable type.
   */
  template <Serializable T>
  IUFHash<HASH>& update(const T& data) {
    const auto size = Serializer<T>::sizeOf(data);
    const auto buf = std::make_unique<unsigned char[]>(size);
    Serializer<T>::write(data, buf.get());
    return update(buf.get(), size);
  }

  /**
   * @brief Finalize and return the digest.
   */
  auto finalize() {
    auto digest = static_cast<HASH*>(this)->write();
    return digest;
  };
};

}  // namespace scl

#endif  // SCL_PRIMITIVES_IUF_HASH_H
