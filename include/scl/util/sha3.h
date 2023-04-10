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

#ifndef SCL_UTIL_SHA3_H
#define SCL_UTIL_SHA3_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "scl/util/digest.h"
#include "scl/util/iuf_hash.h"

namespace scl::util {

/**
 * @brief SHA3 hash function.
 * @tparam DigestSize the output size in bits. Must be either 256, 384 or 512
 */
template <std::size_t DigestSize>
class Sha3 final : public IUFHash<Sha3<DigestSize>> {
  static_assert(DigestSize == 256 || DigestSize == 384 || DigestSize == 512,
                "Invalid SHA3 digest size. Must be 256, 384 or 512");

 public:
  /**
   * @brief The type of a SHA3 digest.
   */
  using DigestType = typename Digest<DigestSize>::Type;

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
  static const std::size_t STATE_SIZE = 25;
  static const std::size_t CAPACITY = 2 * DigestSize / (8 * sizeof(uint64_t));
  static const std::size_t CUTTOFF = STATE_SIZE - (CAPACITY & (~0x80000000));

  uint64_t m_state[STATE_SIZE] = {0};
  unsigned char m_state_bytes[STATE_SIZE * 8] = {0};
  uint64_t m_saved = 0;
  unsigned int m_byte_index = 0;
  unsigned int m_word_index = 0;
};

/**
 * @brief Keccak function.
 * @param state the current state
 */
void Keccakf(uint64_t state[25]);

template <std::size_t DigestSize>
void Sha3<DigestSize>::Hash(const unsigned char* bytes, std::size_t nbytes) {
  unsigned int old_tail = (8 - m_byte_index) & 7;
  const unsigned char* p = bytes;

  if (nbytes < old_tail) {
    while (nbytes-- > 0) {
      m_saved |= (uint64_t)(*(p++)) << ((m_byte_index++) * 8);
    }
    return;
  }

  if (old_tail != 0) {
    nbytes -= old_tail;
    while (old_tail-- != 0) {
      m_saved |= (uint64_t)(*(p++)) << ((m_byte_index++) * 8);
    }

    m_state[m_word_index] ^= m_saved;
    m_byte_index = 0;
    m_saved = 0;

    if (++m_word_index == CUTTOFF) {
      Keccakf(m_state);
      m_word_index = 0;
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

    m_state[m_word_index] ^= t;

    if (++m_word_index == CUTTOFF) {
      Keccakf(m_state);
      m_word_index = 0;
    }
    p += sizeof(uint64_t);
  }

  while (tail-- > 0) {
    m_saved |= (uint64_t)(*(p++)) << ((m_byte_index++) * 8);
  }
}

template <std::size_t DigestSize>
auto Sha3<DigestSize>::Write() -> Sha3<DigestSize>::DigestType {
  uint64_t t = (uint64_t)(((uint64_t)(0x02 | (1 << 2))) << ((m_byte_index)*8));
  m_state[m_word_index] ^= m_saved ^ t;
  m_state[CUTTOFF - 1] ^= 0x8000000000000000ULL;
  Keccakf(m_state);

  for (std::size_t i = 0; i < STATE_SIZE; ++i) {
    const unsigned int t1 = (uint32_t)m_state[i];
    const unsigned int t2 = (uint32_t)((m_state[i] >> 16) >> 16);
    m_state_bytes[i * 8 + 0] = (unsigned char)t1;
    m_state_bytes[i * 8 + 1] = (unsigned char)(t1 >> 8);
    m_state_bytes[i * 8 + 2] = (unsigned char)(t1 >> 16);
    m_state_bytes[i * 8 + 3] = (unsigned char)(t1 >> 24);
    m_state_bytes[i * 8 + 4] = (unsigned char)t2;
    m_state_bytes[i * 8 + 5] = (unsigned char)(t2 >> 8);
    m_state_bytes[i * 8 + 6] = (unsigned char)(t2 >> 16);
    m_state_bytes[i * 8 + 7] = (unsigned char)(t2 >> 24);
  }

  // truncate
  DigestType digest = {0};
  for (std::size_t i = 0; i < digest.size(); ++i) {
    digest[i] = m_state_bytes[i];
  }

  return digest;
}

}  // namespace scl::util

#endif  // SCL_UTIL_SHA3_H
