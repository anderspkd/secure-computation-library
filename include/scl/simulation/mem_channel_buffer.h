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

#ifndef SCL_SIMULATION_MEM_CHANNEL_BUFFER_H
#define SCL_SIMULATION_MEM_CHANNEL_BUFFER_H

#include <algorithm>
#include <array>
#include <memory>
#include <vector>

#include "scl/simulation/buffer.h"

namespace scl::sim {

/**
 * @brief A channel buffer backed by in-memory vectors.
 *
 * MemoryBackedChannelBuffer works much the same as MemoryBackedChannel, in that
 * it internally just holds two vectors. One for reading, and one for
 * writing. The difference is that MemoryBackedChannelBuffer allows for writes
 * and reads to be rolled back.
 */
class MemoryBackedChannelBuffer final : public ChannelBuffer {
 public:
  /**
   * @brief Create a channel buffer connected to itself.
   */
  static std::shared_ptr<ChannelBuffer> CreateLoopback() {
    auto buf = std::make_shared<std::vector<unsigned char>>();
    return std::make_shared<MemoryBackedChannelBuffer>(buf, buf);
  }

  /**
   * @brief Create a pair of paired channels.
   */
  static std::array<std::shared_ptr<ChannelBuffer>, 2> CreatePaired() {
    auto buf0 = std::make_shared<std::vector<unsigned char>>();
    auto buf1 = std::make_shared<std::vector<unsigned char>>();
    return {std::make_shared<MemoryBackedChannelBuffer>(buf0, buf1),
            std::make_shared<MemoryBackedChannelBuffer>(buf1, buf0)};
  }

  /**
   * @brief Create a Memory backed ChannelBuffer.
   * @param write_buffer buffer for storing writes
   * @param read_buffer buffer for storing reads
   */
  MemoryBackedChannelBuffer(
      std::shared_ptr<std::vector<unsigned char>> write_buffer,
      std::shared_ptr<std::vector<unsigned char>> read_buffer)
      : m_write_buf(write_buffer),
        m_read_buf(read_buffer),
        m_write_ptr(0),
        m_read_ptr(0){};

  ~MemoryBackedChannelBuffer() {}

  std::size_t Size() override {
    return m_read_buf->size() - m_read_ptr;
  }

  std::vector<unsigned char> Read(std::size_t n) override {
    // silence clang-tidy
    auto m = (std::vector<unsigned char>::difference_type)m_read_ptr;
    auto n_ = (std::vector<unsigned char>::difference_type)n;
    std::vector<unsigned char> data{m_read_buf->begin() + m,
                                    m_read_buf->begin() + m + n_};
    m_read_ptr += n;
    return data;
  }

  void Write(const std::vector<unsigned char>& data) override {
    m_write_buf->insert(m_write_buf->end(), data.begin(), data.end());
  }

  void Prepare() override {
    m_write_ptr = m_write_buf->size();
    m_read_ptr = 0;
  }

  void Commit() override {
    // erase the data that was read since Prepare and reset write/read ptr.
    auto m = (std::vector<unsigned char>::difference_type)m_read_ptr;
    m_read_buf->erase(m_read_buf->begin(), m_read_buf->begin() + m);

    m_read_ptr = 0;
    m_write_ptr = m_write_buf->size();
  }

  void Rollback() override {
    // erase data written since Prepare and reset the read ptr.
    m_write_buf->resize(m_write_ptr);
    m_read_ptr = 0;
  }

 private:
  std::shared_ptr<std::vector<unsigned char>> m_write_buf;
  std::shared_ptr<std::vector<unsigned char>> m_read_buf;

  std::size_t m_write_ptr;
  std::size_t m_read_ptr;
};

}  // namespace scl::sim

#endif  // SCL_SIMULATION_MEM_CHANNEL_BUFFER_H
