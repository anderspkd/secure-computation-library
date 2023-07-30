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

#ifndef SCL_NET_CHANNEL_H
#define SCL_NET_CHANNEL_H

#include <cstring>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "scl/net/config.h"
#include "scl/net/packet.h"
#include "scl/util/traits.h"

namespace scl::net {

/**
 * @brief Channel interface.
 *
 * Channel defines the interface for a channel between two peers, as well
 * as a number of convenience methods for sending and receiving different kinds
 * of data. To implement an actual channel, subclass Channel and implement
 * the four virtual methods.
 *
 * @see InMemoryChannel
 * @see TcpChannel
 */
class Channel {
 public:
  virtual ~Channel(){};

  /**
   * @brief Close connection to remote.
   */
  virtual void Close() = 0;

  /**
   * @brief Send data to the remote party.
   * @param src the data to send
   * @param n the number of bytes to send
   */
  virtual void Send(const unsigned char* src, std::size_t n) = 0;

  /**
   * @brief Receive data from the remote party.
   * @param dst where to store the received data
   * @param n how much data to receive
   * @return how many bytes were received.
   */
  virtual std::size_t Recv(unsigned char* dst, std::size_t n) = 0;

  /**
   * @brief Check if there is something to receive on this channel.
   * @return true if this channel has data and false otherwise.
   */
  virtual bool HasData() = 0;

  /**
   * @brief Send a Packet on this channel.
   * @param packet the packet.
   *
   * The default implementation of this function makes two calls to Send. One
   * for sending the size of the packet, and one for sendin the content.
   */
  virtual void Send(const Packet& packet);

  /**
   * @brief Receive a Packet from this channel.
   * @param block whether to block until the packet has been received.
   * @return a packet. May return nothing when \p block is true.
   *
   * The default implementation of this function will call Recv and HasData in
   * case \p block is true. When receiving a packet in blocking mode, Recv is
   * called immidiately. Otherwise, HasData will be called first to determine if
   * there's any data available.
   */
  virtual std::optional<Packet> Recv(bool block = true);
};

}  // namespace scl::net

#endif  // SCL_NET_CHANNEL_H
