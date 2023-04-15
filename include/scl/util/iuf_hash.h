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

#ifndef SCL_UTIL_IUF_HASH_H
#define SCL_UTIL_IUF_HASH_H

#include <array>
#include <cstdint>
#include <iomanip>
#include <string>
#include <vector>

namespace scl::util {

/**
 * @brief IUF interface for hash functions.
 * @tparam HashImpl hash implementation
 * @see Sha3
 * @see Sha256
 */
template <typename HashImpl>
struct IUFHash {
  /**
   * @brief Update the hash function with a set of bytes.
   * @param bytes a pointer to a number of bytes.
   * @param n the number of bytes.
   * @return the updated Hash object.
   */
  IUFHash<HashImpl>& Update(const unsigned char* bytes, std::size_t n) {
    static_cast<HashImpl*>(this)->Hash(bytes, n);
    return *this;
  };

  /**
   * @brief Update the hash function with the content from a byte vector.
   * @param data a vector of bytes.
   * @return the updated Hash object.
   */
  IUFHash<HashImpl>& Update(const std::vector<unsigned char>& data) {
    return Update(data.data(), data.size());
  };

  /**
   * @brief Update the hash function with the content from a byte STL array.
   * @param data the array
   * @return the updated Hash object.
   */
  template <std::size_t N>
  IUFHash<HashImpl>& Update(const std::array<unsigned char, N>& data) {
    return Update(data.data(), N);
  }

  /**
   * @brief Update the hash function with the content of a string.
   * @param string the string.
   * @return the updated Hash object.
   */
  IUFHash<HashImpl>& Update(std::string_view string) {
    return Update(reinterpret_cast<const unsigned char*>(string.data()),
                  string.size());
  }

  /**
   * @brief Finalize and return the digest.
   * @return a digest.
   */
  auto Finalize() {
    auto digest = static_cast<HashImpl*>(this)->Write();
    return digest;
  };
};

}  // namespace scl::util

#endif  // SCL_UTIL_IUF_HASH_H
