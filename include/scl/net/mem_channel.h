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

#ifndef SCL_NET_MEM_CHANNEL_H
#define SCL_NET_MEM_CHANNEL_H

#include <array>
#include <memory>

#include "scl/net/channel.h"
#include "scl/net/shared_deque.h"

namespace scl::net {

/**
 * @brief Channel that communicates through in-memory buffers.
 */
class MemoryBackedChannel final : public Channel {
 private:
  using Buffer = SharedDeque<std::vector<unsigned char>>;

 public:
  /**
   * @brief Create a pair of paired channels.
   *
   * This method returns a pair of channels that shared their buffers such that
   * what is sent on one can be retrieved on the other.
   */
  static std::array<std::shared_ptr<MemoryBackedChannel>, 2> CreatePaired() {
    auto buf0 = std::make_shared<Buffer>();
    auto buf1 = std::make_shared<Buffer>();
    auto chl0 = std::make_shared<MemoryBackedChannel>(buf0, buf1);
    auto chl1 = std::make_shared<MemoryBackedChannel>(buf1, buf0);
    return {chl0, chl1};
  };

  /**
   * @brief Create a channel that sends to itself.
   */
  static std::shared_ptr<MemoryBackedChannel> CreateLoopback() {
    auto buf = std::make_shared<Buffer>();
    return std::make_shared<MemoryBackedChannel>(buf, buf);
  }

  /**
   * @brief Create a new channel that sends and receives on in-memory buffers.
   * @param in_buffer the buffer to read incoming messages from
   * @param out_buffer the buffer to put outgoing messages
   */
  MemoryBackedChannel(std::shared_ptr<Buffer> in_buffer,
                      std::shared_ptr<Buffer> out_buffer)
      : mIn(std::move(in_buffer)), mOut(std::move(out_buffer)){};

  ~MemoryBackedChannel(){};

  void Send(const unsigned char* src, std::size_t n) override;
  std::size_t Recv(unsigned char* dst, std::size_t n) override;

  bool HasData() override {
    return mIn->Size() > 0 || !mOverflow.empty();
  };

  void Close() override{};

 private:
  std::shared_ptr<Buffer> mIn;
  std::shared_ptr<Buffer> mOut;
  std::vector<unsigned char> mOverflow;
};

}  // namespace scl::net

#endif  // SCL_NET_MEM_CHANNEL_H
