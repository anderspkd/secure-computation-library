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

#include "scl/net/channel.h"

namespace scl {

/**
 * @brief A channel implementation using TCP.
 * @ingroup net-tcp
 */
class TcpChannel final : public Channel {
 public:
  /**
   * @brief Create a new TcpChannel.
   * @param socket the socket.
   */
  TcpChannel(int socket) : m_alive(true), m_socket(socket) {}

  /**
   * @brief Check if this channel is alive.
   *
   * The channel is considered alive upon construction, and dead after the first
   * call to close().
   */
  bool alive() const {
    return m_alive;
  }

  /**
   * @brief Close this channel.
   *
   * Calling close() will result in all future calls to alive() returning
   * false. This function can be called multiple times.
   */
  void close() override;

  /**
   * @brief Send a packet on this channel.
   * @param packet the packet to send.
   *
   * This function will attempt to send \p packet on the underlying socket. If
   * this would result in the call blocking, then the function is suspended and
   * scheduled to run later through the supplied scheduler.
   */
  Task<void> send(Packet&& packet) override;

  /**
   * @brief Send a packet on this channel.
   * @param packet the packet to send.
   *
   * This function will attempt to send \p packet on the underlying socket. If
   * this would result in the call blocking, then the function is suspended and
   * scheduled to run later through the supplied scheduler.
   */
  Task<void> send(const Packet& packet) override;

  /**
   * @brief Recv a packet on this channel.
   * @return the received packet.
   *
   * This function will suspend execution if not enough data is ready yet. To
   * check if it's possible to receive something on the channel, use poll().
   */
  Task<Packet> recv() override;

  /**
   * @brief Check if this channel has data ready for recovering.
   * @return true if there's data to receive and false otherwise.
   */
  Task<bool> poll() override;

 private:
  bool m_alive;
  int m_socket;
};

}  // namespace scl
