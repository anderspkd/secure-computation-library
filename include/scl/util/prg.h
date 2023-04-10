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

#ifndef SCL_UTIL_PRG_H
#define SCL_UTIL_PRG_H

#include <array>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <emmintrin.h>
#include <wmmintrin.h>

/**
 * @brief 64 bit nonce which is prepended to the counter in the PRG.
 */
#ifndef PRG_NONCE
#define PRG_NONCE 0x0123456789ABCDEF
#endif

/**
 * @brief The initial value of the PRG counter.
 */
#ifndef PRG_INITIAL_COUNTER
#define PRG_INITIAL_COUNTER 0
#endif

namespace scl::util {

/**
 * @brief Pseudorandom generator based on AES-CTR.
 *
 * The <code>PRG</code> class implements a pseudorandom generator based on
 * AES-CTR where the n'th block of output is generated by encrypting a
 * counter. A block of random data is generated by computing
 *
 *   <code>block := AES(seed, counter)</code>
 *
 * where <code>seed</code> is either all 0s or a user supplied value, and
 * <code>counter</code> is a 128-bit counter initialized to be
 *
 *  <code>counter_init := PRG_NONCE || 0 ... 0</code>
 *
 * where each half is 64 bits. The value of PRG_NONCE can be set by defining it
 * as a macro. It defaults to <code>0x0123456789ABCDEF</code>.
 */
class PRG {
 private:
  using BlockType = __m128i;
  static constexpr std::size_t BLOCK_SIZE = sizeof(BlockType);

 public:
  /**
   * @brief Size of the seed.
   */
  static constexpr std::size_t SeedSize() {
    return BLOCK_SIZE;
  };

  /**
   * @brief Create a new PRG with seed 0.
   */
  static PRG Create();

  /**
   * @brief Create a new PRG with a provided seed.
   * @param seed the seed.
   * @param seed_len length of the seed
   */
  static PRG Create(const unsigned char* seed, std::size_t seed_len);

  /**
   * @brief Create a new PRG from a provided seed.
   * @param seed the seed.
   */
  static PRG Create(const std::string& seed);

  /**
   * @brief Reset the PRG.
   *
   * This method allows resetting a PRG object to its initial state.
   */
  void Reset();

  /**
   * @brief Generate random data and store it in a supplied buffer.
   * @param buffer the buffer
   * @param n how many bytes of random data to generate
   */
  void Next(unsigned char* buffer, std::size_t n);

  /**
   * @brief Generate random data and store it in a supplied buffer.
   * @param buffer the buffer with space pre-allocated
   *
   * How many bytes of random data to generate is decided based on the output of
   * <code>buffer.size()</code>.
   */
  void Next(std::vector<unsigned char>& buffer) {
    Next(buffer.data(), buffer.size());
  };

  /**
   * @brief Generate random data and store in in a supplied buffer.
   * @param buffer the buffer
   * @param n how many random bytes to generate
   * @throws std::invalid_argument if \p n is greater than
   * <code>buffer.size()</code>.
   *
   * The capacity of \p buffer is not affected in any way by this method and it
   * requires that it has room for at least \p n elements.
   */
  void Next(std::vector<unsigned char>& buffer, std::size_t n) {
    if (buffer.size() < n) {
      throw std::invalid_argument("n exceeds buffer.size()");
    }
    Next(buffer.data(), n);
  };

  /**
   * @brief Generate and return random data.
   * @param n the number of random bytes to generate
   * @return the random bytes.
   */
  std::vector<unsigned char> Next(std::size_t n) {
    auto buffer = std::make_unique<unsigned char[]>(n);
    Next(buffer.get(), n);
    return std::vector<unsigned char>(buffer.get(), buffer.get() + n);
  };

  /**
   * @brief The seed.
   */
  std::array<unsigned char, BLOCK_SIZE> Seed() const {
    return m_seed;
  };

 private:
  PRG(std::array<unsigned char, BLOCK_SIZE> seed) : m_seed(seed){};

  std::array<unsigned char, BLOCK_SIZE> m_seed = {0};
  long m_counter = PRG_INITIAL_COUNTER;
  BlockType m_state[11];

  void Update();
  void Init();
};

}  // namespace scl::util

#endif  // SCL_UTIL_PRG_H
