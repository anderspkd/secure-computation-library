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

#pragma once

#include <bitset>
#include <cstddef>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <vector>

#include "scl/serialization.h"

namespace scl {

/**
 * @brief A bitmap.
 * @ingroup util
 *
 * Bitmap serves a similar purpose as <code>std::vector<bool></code>, but
 * behaves a bit more like a "fixed-length vector of bits". The main
 * differentiator is that Bitmap implements various element-wise operations.
 *
 * Bitmap is a useful class for protocols that makes use of indicator sets,
 * because that's effectively what Bitmap is.
 *
 * @code
 * Bitmap bm(10);    // this creates a Bitmap with space for 10 bits, with
 *                   // all 10 bits initialized to be 0.
 *
 * assert(bm.size() == 10);
 * assert(bm.numberOfBlocks() == ...);  // a multiple of Bitmap::BITS_PER_BLOCK
 *
 * assert(bm.count() == 0)
 * bm.set(0, true);  // set the first bit to be 1
 * assert(bm.count() == 1);
 * assert(bm.at(0));
 * assert(!bm.at(1));
 *
 * std::cout << bm;
 * // prints "0000000000000001"
 * // note that a multiple of sizeof(Bitmap::BlockType) is printed.
 *
 * Bitmap bm1(10);
 * Bitmap bm2(10);
 *
 * bm1.set(0, 1);
 * bm1.set(1, 1);
 * bm2.set(1, 1);
 * bm2.set(2, 1);
 *
 * assert((bm1 & bm2).count() == 1);
 * assert((bm1 ^ bm2).count() == 2);
 * assert((bm1 | bm2).count() == 3);
 *
 * assert((~bm1).count() == 8);
 *
 * // Bitmap::set performs no bounds checking, so the following is perfectly
 * // fine. However, it will lead to weird behaviour. For example
 *
 * Bitmap bm_oob(10);
 * bm_oob.set(10, true);
 * assert(bm_oob.count() == 1);
 * assert((~bm_oob).count() == 10);
 *
 * // Negation does not touch the bits beyond Bitmap::size in order to ensure
 * // that Bitmap::count works as expected. So if we assign bits beyond
 * // Bitmap::size then Bitmap::count might not work correctly anymore :)
 *
 * @endcode
 */
class Bitmap {
 public:
  /**
   * @brief The internal block type.
   */
  using BlockType = unsigned char;

  /**
   * @brief Number of bits that each block stores.
   */
  constexpr static std::size_t BITS_PER_BLOCK = sizeof(BlockType) * 8;

 private:
  using ContainerType = std::vector<BlockType>;

 public:
  /**
   * @brief Create a Bitmap from an STL vector of booleans.
   */
  static Bitmap fromStdVecBool(const std::vector<bool>& bool_vec) {
    Bitmap bm(bool_vec.size());
    for (std::size_t i = 0; i < bool_vec.size(); ++i) {
      bm.set(i, bool_vec[i]);
    }
    return bm;
  }

  /**
   * @brief Construct a Bitmap with some initial size.
   */
  Bitmap(std::size_t initial_size)
      : m_bits(ContainerType(bytesRequired(initial_size), 0)),
        m_true_size(initial_size) {}

  /**
   * @brief Construct an empty Bitmap.
   */
  Bitmap() : Bitmap(0) {}

  /**
   * @brief Check the bit at some position.
   */
  bool at(std::size_t index) const {
    const std::size_t block = index / BITS_PER_BLOCK;
    const std::size_t block_index = index & (BITS_PER_BLOCK - 1);
    return ((m_bits[block] >> block_index) & 1) == 1;
  }

  /**
   * @brief Set the bit at some position.
   */
  void set(std::size_t index, bool b) {
    const std::size_t block = index / BITS_PER_BLOCK;
    const std::size_t block_index = index & (BITS_PER_BLOCK - 1);
    m_bits[block] ^= (-(b ? 1 : 0) ^ m_bits[block]) & (1 << block_index);
  }

  /**
   * @brief Count the number of bits set in this Bitmap.
   */
  std::size_t count() const {
    // https://stackoverflow.com/a/698108
    static const unsigned char lut[] =
        {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};
    std::size_t count = 0;
    for (const auto& b : m_bits) {
      count += lut[b & 0x0f] + lut[b >> 4];
    }
    return count;
  }

  /**
   * @brief The number of blocks this Bitmap uses.
   */
  std::size_t numberOfBlocks() const {
    return m_bits.size();
  }

  /**
   * @brief The size of the bitmap.
   */
  std::size_t size() const {
    return m_true_size;
  }

  /**
   * @brief Check if two bitmaps contain the same content.
   */
  friend bool operator==(const Bitmap& bm0, const Bitmap& bm1) {
    return bm0.m_bits == bm1.m_bits;
  }

  /**
   * @brief Check if two bitmaps are different.
   */
  friend bool operator!=(const Bitmap& bm0, const Bitmap& bm1) {
    return !(bm0 == bm1);
  }

  /**
   * @brief Write this bitmap to a stream.
   */
  friend std::ostream& operator<<(std::ostream& os, const Bitmap& m) {
    for (const BlockType& block : m.m_bits) {
      os << std::bitset<BITS_PER_BLOCK>(block);
    }
    return os;
  }

  /**
   * @brief Compute the XOR of two bitmaps.
   */
  friend Bitmap operator^(const Bitmap& bm0, const Bitmap& bm1) {
    validateSizes(bm0, bm1);
    Bitmap bm(bm0.m_true_size);
    for (std::size_t i = 0; i < bm.m_bits.size(); i++) {
      bm.m_bits[i] = bm0.m_bits[i] ^ bm1.m_bits[i];
    }
    return bm;
  }

  /**
   * @brief Compute the AND of two bitmaps.
   */
  friend Bitmap operator&(const Bitmap& bm0, const Bitmap& bm1) {
    validateSizes(bm0, bm1);
    Bitmap bm(bm0.m_true_size);
    for (std::size_t i = 0; i < bm.m_bits.size(); i++) {
      bm.m_bits[i] = bm0.m_bits[i] & bm1.m_bits[i];
    }
    return bm;
  }

  /**
   * @brief Compute the OR of two bitmaps.
   */
  friend Bitmap operator|(const Bitmap& bm0, const Bitmap& bm1) {
    validateSizes(bm0, bm1);
    Bitmap bm(bm0.m_true_size);
    for (std::size_t i = 0; i < bm.m_bits.size(); i++) {
      bm.m_bits[i] = bm0.m_bits[i] | bm1.m_bits[i];
    }
    return bm;
  }

  /**
   * @brief Compute the negation of a bitmap.
   */
  friend Bitmap operator~(const Bitmap& bm0) {
    Bitmap bm(bm0.m_true_size);
    std::size_t i = 0;
    for (; i < bm.m_bits.size() - 1; i++) {
      bm.m_bits[i] = ~bm0.m_bits[i];
    }
    // for the last block we only negate some of the bits, potentially.
    const auto mask = (1 << (bm.m_true_size % BITS_PER_BLOCK)) - 1;
    bm.m_bits[i] = ~bm0.m_bits[i] & mask;
    return bm;
  }

 private:
  friend Serializer<Bitmap>;

  ContainerType m_bits;
  std::size_t m_true_size;

  static constexpr std::size_t bytesRequired(std::size_t bits) {
    return bits == 0 ? 1 : (bits - 1) / (BITS_PER_BLOCK) + 1;
  }

  static void validateSizes(const Bitmap& bm0, const Bitmap& bm1) {
    if (bm0.size() != bm1.size()) {
      throw std::logic_error("bitmaps are different sizes");
    }
  }
};

/**
 * @brief Serializer for util::Bitmap types.
 */
template <>
struct Serializer<Bitmap> {
  static std::size_t sizeOf(const Bitmap& bm) {
    return Serializer<Bitmap::ContainerType>::sizeOf(bm.m_bits);
  }
  static std::size_t write(const Bitmap& bm, unsigned char* buf) {
    return Serializer<Bitmap::ContainerType>::write(bm.m_bits, buf);
  }
  static std::size_t read(Bitmap& bm, const unsigned char* buf) {
    return Serializer<Bitmap::ContainerType>::read(bm.m_bits, buf);
  }
};

}  // namespace scl
