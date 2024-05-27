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

#ifndef SCL_NET_CHANNEL_H
#define SCL_NET_CHANNEL_H

#include "scl/coro/task.h"
#include "scl/net/packet.h"

namespace scl::net {

/**
 * @brief Peer-to-peer communication channel interface.
 */
class Channel {
 public:
  virtual ~Channel() {}

  /**
   * @brief Close connection to remote.
   */
  virtual void close() = 0;

  /**
   * @brief Send a data packet on the channel.
   * @param packet the packet to send.
   */
  virtual coro::Task<void> send(Packet&& packet) = 0;

  /**
   * @brief Send a data packet on the channel.
   * @param packet the packet to send.
   */
  virtual coro::Task<void> send(const Packet& packet) = 0;

  /**
   * @brief Receive a data packet from on the channel.
   * @return the received packet.
   */
  virtual coro::Task<Packet> recv() = 0;

  /**
   * @brief Check if there is something to receive on this channel.
   * @return true if this channel has data and false otherwise.
   */
  virtual coro::Task<bool> hasData() = 0;
};

}  // namespace scl::net

#endif  // SCL_NET_CHANNEL_H
