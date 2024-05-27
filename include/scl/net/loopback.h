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

#ifndef SCL_NET_LOOPBACK_H
#define SCL_NET_LOOPBACK_H

#include <deque>
#include <memory>
#include <stdexcept>

#include "scl/coro/task.h"
#include "scl/net/channel.h"

namespace scl::net {

/**
 * @brief A loopback channel.
 *
 * This Channel implementation defines a channel which connects to in-memory
 * buffers. This channel is useful as a channel used by a party that talks with
 * itself, or as a channel between two parties that are simply run in-memory.
 */
class LoopbackChannel final : public Channel {
 public:
  /**
   * @brief The type of the internal buffer.
   */
  using Buffer = std::deque<Packet>;

  /**
   * @brief Create a pair of connected channels.
   *
   * This creates two channels <code>ch0</code> and <code>ch1</code> such that
   * any packet sent on <code>ch0</code> can be received on <code>ch1</code>,
   * and vice versa.
   */
  static std::array<std::shared_ptr<Channel>, 2> createPaired() {
    auto buf0 = std::make_shared<Buffer>();
    auto buf1 = std::make_shared<Buffer>();
    return {std::make_shared<LoopbackChannel>(buf0, buf1),
            std::make_shared<LoopbackChannel>(buf1, buf0)};
  }

  /**
   * @brief Create a loopback channel that connects to itself.
   *
   * A channel created with this function will receive anything that it sends.
   */
  static std::shared_ptr<Channel> create() {
    auto buf = std::make_shared<Buffer>();
    return std::make_shared<LoopbackChannel>(buf, buf);
  }

  /**
   * @brief Construct a new loopback channel that is not connected to anything.
   */
  LoopbackChannel() {}

  /**
   * @brief Construct a new loopback channel from a pair of buffers.
   */
  LoopbackChannel(std::shared_ptr<Buffer> in, std::shared_ptr<Buffer> out)
      : m_in(in), m_out(out) {}

  /**
   * @brief Close the channel. Does nothing.
   */
  void close() override {}

  /**
   * @brief Send a packet on this channel.
   *
   * This function will complete immediately once awaited.
   */
  coro::Task<void> send(Packet&& packet) override {
    m_out->emplace_back(packet);
    co_return;
  }

  /**
   * @brief Send a packet on this channel.
   *
   * This function will complete immediately once awaited.
   */
  coro::Task<void> send(const Packet& packet) override {
    m_out->push_back(packet);
    co_return;
  }

  /**
   * @brief Receive a packet on this channel.
   *
   * This function may suspend if there is no data yet.
   */
  coro::Task<Packet> recv() override {
    // suspend in case there is no packets yet
    co_await [this]() { return !this->m_in->empty(); };
    auto packet = m_in->front();
    m_in->pop_front();
    co_return packet;
  }

  /**
   * @brief Check if there are data available for receiving.
   */
  coro::Task<bool> hasData() override {
    co_return !m_in->empty();
  }

  /**
   * @brief Get the size of the next packet.
   *
   * This function does not perform any checks on the incoming buffer.
   */
  std::size_t getNextPacketSize() const {
    return m_in->front().size();
  }

 private:
  std::shared_ptr<Buffer> m_in;
  std::shared_ptr<Buffer> m_out;
};

}  // namespace scl::net

#endif  // SCL_NET_LOOPBACK_H
