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
      : mWriteBuffer(write_buffer),
        mReadBuffer(read_buffer),
        mWritePtr(0),
        mReadPtr(0){};

  ~MemoryBackedChannelBuffer() {}

  std::size_t Size() override {
    return mReadBuffer->size() - mReadPtr;
  }

  std::vector<unsigned char> Read(std::size_t n) override {
    // silence clang-tidy
    auto m = (std::vector<unsigned char>::difference_type)mReadPtr;
    auto n_ = (std::vector<unsigned char>::difference_type)n;
    std::vector<unsigned char> data{mReadBuffer->begin() + m,
                                    mReadBuffer->begin() + m + n_};
    mReadPtr += n;
    return data;
  }

  void Write(const std::vector<unsigned char>& data) override {
    mWriteBuffer->insert(mWriteBuffer->end(), data.begin(), data.end());
  }

  void Prepare() override {
    mWritePtr = mWriteBuffer->size();
    mReadPtr = 0;
  }

  void Commit() override {
    // erase the data that was read since Prepare and reset write/read ptr.
    auto m = (std::vector<unsigned char>::difference_type)mReadPtr;
    mReadBuffer->erase(mReadBuffer->begin(), mReadBuffer->begin() + m);

    mReadPtr = 0;
    mWritePtr = mWriteBuffer->size();
  }

  void Rollback() override {
    // erase data written since Prepare and reset the read ptr.
    mWriteBuffer->resize(mWritePtr);
    mReadPtr = 0;
  }

 private:
  std::shared_ptr<std::vector<unsigned char>> mWriteBuffer;
  std::shared_ptr<std::vector<unsigned char>> mReadBuffer;

  std::size_t mWritePtr;
  std::size_t mReadPtr;
};

}  // namespace scl::sim

#endif  // SCL_SIMULATION_MEM_CHANNEL_BUFFER_H
