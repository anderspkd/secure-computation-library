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

#ifndef SCL_UTIL_BITMAP_H
#define SCL_UTIL_BITMAP_H

#include <bitset>
#include <cstddef>
#include <ostream>
#include <stdexcept>
#include <vector>

#include "scl/serialization/serializer.h"

namespace scl {
namespace util {

/**
 * @brief A simple bitmap.
 *
 * The Bitmap class holds bits. It serves some of the same functionality as
 * <code>std::vector<bool></code>. The implementation of Bitmap stores bits
 * packed in objects of type Bitmap::BlockType, current <code>unsigned
 * char</code>. As a consequence, Bitmap always stores a multiple of
 * <code>sizeof(Bitmap::BlockType) * 8</code> bits. Any unset bits are
 * guaranteed to be 0.
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
   * @brief Create a Bitmap from an <code>std::vector<bool></code>.
   * @param bool_vec the <code>std::vector<bool></code>.
   * @return a Bitmap.
   */
  static Bitmap fromStdVecBool(const std::vector<bool>& bool_vec) {
    Bitmap bm(bool_vec.size());
    for (std::size_t i = 0; i < bool_vec.size(); ++i) {
      bm.set(i, bool_vec[i]);
    }
    return bm;
  }  // LCOV_EXCL_LINE

  /**
   * @brief Construct a Bitmap with some initial size.
   * @param initial_size the initial size.
   */
  Bitmap(std::size_t initial_size)
      : m_bits(ContainerType(bytesRequired(initial_size), 0)) {}

  /**
   * @brief Construct an empty Bitmap.
   */
  Bitmap() : Bitmap(0) {}

  /**
   * @brief Check the bit at some position.
   * @param index the bit position.
   * @return true if the bit at position \p index is set and 0 otherwise.
   */
  bool at(std::size_t index) const {
    const std::size_t block = index / BITS_PER_BLOCK;
    const std::size_t block_index = index & (BITS_PER_BLOCK - 1);
    return ((m_bits[block] >> block_index) & 1) == 1;
  }

  /**
   * @brief Set the bit at some position.
   * @param index the position of the bit to set.
   * @param b the value to set.
   */
  void set(std::size_t index, bool b) {
    const std::size_t block = index / BITS_PER_BLOCK;
    const std::size_t block_index = index & (BITS_PER_BLOCK - 1);
    m_bits[block] ^= (-(b ? 1 : 0) ^ m_bits[block]) & (1 << block_index);
  }

  /**
   * @brief Count the number of bits set in this Bitmap.
   * @return the population count of this Bitmap.
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
   * @brief Get the number of blocks this Bitmap uses.
   * @return the number of BlockType elements used by this Bitmap.
   */
  std::size_t numberOfBlocks() const {
    return m_bits.size();
  }

  /**
   * @brief Check if two bitmaps contain the same content.
   * @param bm0 the first Bitmap.
   * @param bm1 the second Bitmap.
   * @return true if \p bm0 and \p bm1 are equal, false otherwise.
   */
  friend bool operator==(const Bitmap& bm0, const Bitmap& bm1) {
    return bm0.m_bits == bm1.m_bits;
  }

  /**
   * @brief Check if two bitmaps are different.
   * @param bm0 the first Bitmap.
   * @param bm1 the second Bitmap.
   * @return false if \p bm0 and \p bm1 are equal, true otherwise.
   */
  friend bool operator!=(const Bitmap& bm0, const Bitmap& bm1) {
    return !(bm0 == bm1);
  }

  /**
   * @brief Write this bitmap to a stream.
   * @param os the stream.
   * @param m the bitmap.
   */
  friend std::ostream& operator<<(std::ostream& os, const Bitmap& m) {
    for (const BlockType& block : m.m_bits) {
      os << std::bitset<BITS_PER_BLOCK>(block);
    }
    return os;
  }

  /**
   * @brief Compute the XOR of two bitmaps.
   * @param bm0 the first bitmap.
   * @param bm1 the other bitmap.
   */
  friend Bitmap operator^(const Bitmap& bm0, const Bitmap& bm1) {
    validateSizes(bm0, bm1);
    Bitmap bm;
    bm.m_bits.resize(bm0.numberOfBlocks());
    for (std::size_t i = 0; i < bm.m_bits.size(); i++) {
      bm.m_bits[i] = bm0.m_bits[i] ^ bm1.m_bits[i];
    }
    return bm;
  }

  /**
   * @brief Compute the AND of two bitmaps.
   * @param bm0 the first bitmap.
   * @param bm1 the other bitmap.
   */
  friend Bitmap operator&(const Bitmap& bm0, const Bitmap& bm1) {
    validateSizes(bm0, bm1);
    Bitmap bm;
    bm.m_bits.resize(bm0.numberOfBlocks());
    for (std::size_t i = 0; i < bm.m_bits.size(); i++) {
      bm.m_bits[i] = bm0.m_bits[i] & bm1.m_bits[i];
    }
    return bm;
  }

  /**
   * @brief Compute the OR of two bitmaps.
   * @param bm0 the first bitmap.
   * @param bm1 the other bitmap.
   */
  friend Bitmap operator|(const Bitmap& bm0, const Bitmap& bm1) {
    validateSizes(bm0, bm1);
    Bitmap bm;
    bm.m_bits.resize(bm0.numberOfBlocks());
    for (std::size_t i = 0; i < bm.m_bits.size(); i++) {
      bm.m_bits[i] = bm0.m_bits[i] | bm1.m_bits[i];
    }
    return bm;
  }

  /**
   * @brief Compute the negation of a bitmap.
   * @param bm0 the bitmap.
   */
  friend Bitmap operator~(const Bitmap& bm0) {
    Bitmap bm;
    bm.m_bits.resize(bm0.numberOfBlocks());
    for (std::size_t i = 0; i < bm.m_bits.size(); i++) {
      bm.m_bits[i] = ~bm0.m_bits[i];
    }
    return bm;
  }

 private:
  ContainerType m_bits;

  static constexpr std::size_t bytesRequired(std::size_t bits) {
    return bits == 0 ? 1 : (bits - 1) / (BITS_PER_BLOCK) + 1;
  }

  static void validateSizes(const util::Bitmap& bm0, const util::Bitmap& bm1) {
    if (bm0.numberOfBlocks() != bm1.numberOfBlocks()) {
      throw std::logic_error("bitmaps are different sizes");
    }
  }

  friend scl::seri::Serializer<Bitmap>;
};

}  // namespace util

namespace seri {

/**
 * @brief Serializer for util::Bitmap types.
 */
template <>
struct Serializer<util::Bitmap> {
  /**
   * @brief Get serialized size of a util::Bitmap.
   * @param bm the util::Bitmap.
   * @return the size in bytes of the \p bm.
   */
  static std::size_t sizeOf(const util::Bitmap& bm) {
    return Serializer<util::Bitmap::ContainerType>::sizeOf(bm.m_bits);
  }

  /**
   * @brief Write a util::Bitmap to a buffer.
   * @param bm the util::Bitmap.
   * @param buf the buffer.
   * @return the number of bytes written.
   */
  static std::size_t write(const util::Bitmap& bm, unsigned char* buf) {
    return Serializer<util::Bitmap::ContainerType>::write(bm.m_bits, buf);
  }

  /**
   * @brief Read a util::Bitmap from a buffer.
   * @param bm the util::Bitmap that will store the result.
   * @param buf the buffer to read the util::Bitmap from.
   * @return the number of bytes read from \p buf.
   */
  static std::size_t read(util::Bitmap& bm, const unsigned char* buf) {
    return Serializer<util::Bitmap::ContainerType>::read(bm.m_bits, buf);
  }
};

}  // namespace seri

}  // namespace scl

#endif  // SCL_UTIL_BITMAP_H
