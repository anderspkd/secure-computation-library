/**
 * @file sha3.h
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

#ifndef SCL_PRIMITIVES_SHA3_H
#define SCL_PRIMITIVES_SHA3_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "scl/primitives/digest.h"
#include "scl/primitives/iuf_hash.h"

namespace scl {
namespace details {

/**
 * @brief SHA3 hash function.
 * @tparam DigestSize the output size in bits. Must be either 256, 384 or 512
 */
template <std::size_t DigestSize>
class Sha3 final : public details::IUFHash<Sha3<DigestSize>> {
  static_assert(DigestSize == 256 || DigestSize == 384 || DigestSize == 512,
                "Invalid SHA3 digest size. Must be 256, 384 or 512");

 public:
  /**
   * @brief The type of a SHA3 digest.
   */
  using DigestType = typename details::Digest<DigestSize>::Type;

  /**
   * @brief Update the hash function with a set of bytes.
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
  static const std::size_t kStateSize = 25;
  static const std::size_t kCapacity = 2 * DigestSize / (8 * sizeof(uint64_t));
  static const std::size_t kCuttoff = kStateSize - (kCapacity & (~0x80000000));

  uint64_t mState[kStateSize] = {0};
  unsigned char mStateBytes[kStateSize * 8] = {0};
  uint64_t mSaved = 0;
  unsigned int mByteIndex = 0;
  unsigned int mWordIndex = 0;
};

/**
 * @brief Keccak function.
 * @param state the current state
 */
void Keccakf(uint64_t state[25]);

template <std::size_t DigestSize>
void Sha3<DigestSize>::Hash(const unsigned char* bytes, std::size_t nbytes) {
  unsigned int old_tail = (8 - mByteIndex) & 7;
  const unsigned char* p = bytes;

  if (nbytes < old_tail) {
    while (nbytes-- > 0) {
      mSaved |= (uint64_t)(*(p++)) << ((mByteIndex++) * 8);
    }
    return;
  }

  if (old_tail != 0) {
    nbytes -= old_tail;
    while (old_tail-- != 0) {
      mSaved |= (uint64_t)(*(p++)) << ((mByteIndex++) * 8);
    }

    mState[mWordIndex] ^= mSaved;
    mByteIndex = 0;
    mSaved = 0;

    if (++mWordIndex == kCuttoff) {
      Keccakf(mState);
      mWordIndex = 0;
    }
  }

  std::size_t words = nbytes / sizeof(uint64_t);
  unsigned int tail = nbytes - words * sizeof(uint64_t);

  for (std::size_t i = 0; i < words; ++i) {
    const uint64_t t =
        (uint64_t)(p[0]) | ((uint64_t)(p[1]) << 8 * 1) |
        ((uint64_t)(p[1]) << 8 * 2) | ((uint64_t)(p[1]) << 8 * 3) |
        ((uint64_t)(p[1]) << 8 * 4) | ((uint64_t)(p[1]) << 8 * 5) |
        ((uint64_t)(p[1]) << 8 * 6) | ((uint64_t)(p[1]) << 8 * 7);

    mState[mWordIndex] ^= t;

    if (++mWordIndex == kCuttoff) {
      Keccakf(mState);
      mWordIndex = 0;
    }
    p += sizeof(uint64_t);
  }

  while (tail-- > 0) {
    mSaved |= (uint64_t)(*(p++)) << ((mByteIndex++) * 8);
  }
}

template <std::size_t DigestSize>
auto Sha3<DigestSize>::Write() -> Sha3<DigestSize>::DigestType {
  uint64_t t = (uint64_t)(((uint64_t)(0x02 | (1 << 2))) << ((mByteIndex)*8));
  mState[mWordIndex] ^= mSaved ^ t;
  mState[kCuttoff - 1] ^= 0x8000000000000000ULL;
  Keccakf(mState);

  for (std::size_t i = 0; i < kStateSize; ++i) {
    const unsigned int t1 = (uint32_t)mState[i];
    const unsigned int t2 = (uint32_t)((mState[i] >> 16) >> 16);
    mStateBytes[i * 8 + 0] = (unsigned char)t1;
    mStateBytes[i * 8 + 1] = (unsigned char)(t1 >> 8);
    mStateBytes[i * 8 + 2] = (unsigned char)(t1 >> 16);
    mStateBytes[i * 8 + 3] = (unsigned char)(t1 >> 24);
    mStateBytes[i * 8 + 4] = (unsigned char)t2;
    mStateBytes[i * 8 + 5] = (unsigned char)(t2 >> 8);
    mStateBytes[i * 8 + 6] = (unsigned char)(t2 >> 16);
    mStateBytes[i * 8 + 7] = (unsigned char)(t2 >> 24);
  }

  // truncate
  DigestType digest = {0};
  for (std::size_t i = 0; i < digest.size(); ++i) {
    digest[i] = mStateBytes[i];
  }

  return digest;
}

}  // namespace details
}  // namespace scl

#endif  // SCL_PRIMITIVES_SHA3_H
