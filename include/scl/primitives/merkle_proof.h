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

#ifndef SCL_UTIL_MERKLE_PROOF_H
#define SCL_UTIL_MERKLE_PROOF_H

#include <vector>

#include "scl/serialization/serializer.h"
#include "scl/util/bitmap.h"

namespace scl {
namespace util {

/**
 * @brief A Merkle tree proof.
 */
template <typename DIGEST>
struct MerkleProof {
  /**
   * @brief The path from a particular leaf to the root.
   */
  std::vector<DIGEST> path;

  /**
   * @brief A vector describing whether at the left or right element for each
   * element in a path.
   */
  Bitmap direction;
};

}  // namespace util

namespace seri {

/**
 * @brief Serializer for MerkleProof.
 */
template <typename DIGEST>
struct Serializer<util::MerkleProof<DIGEST>> {
  /**
   * @brief Determines the size in bytes of a merkle proof.
   * @param proof the proof.
   * @return the size of \p proof in bytes.
   */
  static std::size_t sizeOf(const util::MerkleProof<DIGEST>& proof) {
    return Serializer<std::vector<DIGEST>>::sizeOf(proof.path) +
           Serializer<util::Bitmap>::sizeOf(proof.direction);
  }

  /**
   * @brief Write a merkle proof to a buffer.
   * @param proof the proof.
   * @param buf the buffer.
   * @return the number of bytes written to \p buf.
   */
  static std::size_t write(const util::MerkleProof<DIGEST>& proof,
                           unsigned char* buf) {
    buf += Serializer<std::vector<DIGEST>>::write(proof.path, buf);
    buf += Serializer<util::Bitmap>::write(proof.direction, buf);
    return sizeOf(proof);
  }

  /**
   * @brief Read a merkle proof from a buffer.
   * @param proof the proof.
   * @param buf the buffer.
   * @return the number of bytes read from \p buf.
   */
  static std::size_t read(util::MerkleProof<DIGEST>& proof,
                          const unsigned char* buf) {
    buf += Serializer<std::vector<DIGEST>>::read(proof.path, buf);
    buf += Serializer<util::Bitmap>::read(proof.direction, buf);
    return sizeOf(proof);
  }
};

}  // namespace seri
}  // namespace scl

#endif  // SCL_UTIL_MERKLE_PROOF_H
