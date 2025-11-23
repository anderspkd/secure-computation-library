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

#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

#include <emmintrin.h>

namespace scl {

/**
 * @brief Pseudorandom generator.
 * @ingroup prim
 *
 * @code
 * #include <scl/primitives/prg.h>
 *
 * auto prg = PRG::create();
 *
 * std::array<unsigned char, 10> buffer;
 * prg.next(buffer);  // buffer is filled with random bytes
 *
 * std::vector<unsigned char> rands = prg.next(42);
 * assert(rands.size() == 42);
 *
 * auto prg1 = PRG::create("seed1");
 * auto prg2 = PRG::create("seed2");
 *
 * assert(prg1.next(42) != prg2.next(42));
 * @endcode
 *
 * The underlying implementation of PRG is based on AES128 in counter mode.
 * When supplying an explicit seed, only the first PRG::seedSize() bytes are
 * used. For the current implementation this means only the first 16 bytes are
 * counted.
 *
 * @code
 * auto prg1 = PRG::create("abcdef0123456789abc");
 * auto prg2 = PRG::create("abcdef0123456789xyz");
 *
 * assert(prg1.next(42) == prg2.next(42));
 * @endcode
 */
class PRG {
 private:
  using BlockType = __m128i;
  static constexpr std::size_t BLOCK_SIZE = sizeof(BlockType);

 public:
  /**
   * @brief Size of the seed.
   */
  static constexpr std::size_t seedSize() {
    return BLOCK_SIZE;
  }

  /**
   * @brief Create a new PRG with seed 0.
   */
  static PRG create();

  /**
   * @brief Create a new PRG with a provided seed.
   */
  static PRG create(const unsigned char* seed, std::size_t seed_len);

  /**
   * @brief Create a new PRG from a provided seed.
   */
  static PRG create(const std::string& seed);

  /**
   * @brief Reset the PRG.
   */
  void reset();

  /**
   * @brief Generate random data and store it in a supplied buffer.
   */
  void next(unsigned char* buffer, std::size_t n);

  /**
   * @brief Generate random data and store it in a supplied buffer.
   */
  void next(std::vector<unsigned char>& buffer) {
    next(buffer.data(), buffer.size());
  }

  /**
   * @brief Generate random data and store it in a supplied buffer.
   */
  template <std::size_t N>
  void next(std::array<unsigned char, N>& buffer) {
    next(buffer.data(), N);
  }

  /**
   * @brief Generate random data and store in in a supplied buffer.
   */
  void next(std::vector<unsigned char>& buffer, std::size_t n) {
    if (buffer.size() < n) {
      throw std::invalid_argument("n exceeds buffer.size()");
    }
    next(buffer.data(), n);
  }

  /**
   * @brief Generate and return random data.
   */
  std::vector<unsigned char> next(std::size_t n) {
    std::vector<unsigned char> buffer;
    buffer.reserve(n);
    next(buffer);
    return buffer;
  }

  /**
   * @brief The seed.
   */
  std::array<unsigned char, BLOCK_SIZE> seed() const {
    return m_seed;
  }

 private:
  std::array<unsigned char, BLOCK_SIZE> m_seed = {0};
  long m_counter = 0;
  BlockType m_state[11];

  PRG(std::array<unsigned char, BLOCK_SIZE> seed) : m_seed(seed) {};

  void update();
  void init();
};

}  // namespace scl
