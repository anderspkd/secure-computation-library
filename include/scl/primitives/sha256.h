/**
 * @file sha256.h
 *
 * SCL --- Secure Computation Library
 * Copyright (C) 2022 Anders Dalskov
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

#ifndef SCL_PRIMITIVES_SHA256_H
#define SCL_PRIMITIVES_SHA256_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "scl/primitives/digest.h"
#include "scl/primitives/iuf_hash.h"

namespace scl {
namespace details {

/**
 * @brief SHA256 hash function.
 */
class Sha256 final : public details::IUFHash<Sha256> {
 public:
  /**
   * @brief The type of a SHA256 digest.
   */
  using DigestType = typename details::Digest<256>::Type;

  /**
   * @brief Update the hash function with a set of bytes.
   *
   * @param bytes a pointer to a number of bytes.
   * @param nbytes the number of bytes.
   * @return the updated Hash object.
   */
  void Hash(const unsigned char* bytes, std::size_t nbytes);

  /**
   * @brief Finalize and return the digest.
   */
  DigestType Write();

 private:
  std::array<unsigned char, 64> mChunk;
  std::uint32_t mChunkPos = 0;
  std::size_t mTotalLen = 0;
  std::array<uint32_t, 8> mState = {0x6a09e667,
                                    0xbb67ae85,
                                    0x3c6ef372,
                                    0xa54ff53a,
                                    0x510e527f,
                                    0x9b05688c,
                                    0x1f83d9ab,
                                    0x5be0cd19};

  void Transform();
  void Pad();
  DigestType WriteDigest();
};

}  // namespace details
}  // namespace scl

#endif  // SCL_PRIMITIVES_SHA256_H
