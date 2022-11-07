/**
 * @file tcp_channel.h
 *
 * SCL --- Secure Computation Library
 * Copyright (C) 2022 Anders Dalskov
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

#ifndef SCL_NET_TCP_CHANNEL_H
#define SCL_NET_TCP_CHANNEL_H

#include <cstring>
#include <memory>
#include <vector>

#include "scl/net/channel.h"
#include "scl/net/config.h"

namespace scl {

/**
 * @brief A channel between two peers utilizing TCP.
 */
class TcpChannel final : public Channel {
 public:
  /**
   * @brief Wrap a socket in a TCP channel.
   * @param socket the socket.
   */
  TcpChannel(int socket) : mAlive(true), mSocket(socket){};

  /**
   * @brief Destroying a TCP channel closes the connection.
   */
  ~TcpChannel() { Close(); };

  /**
   * @brief Tells whether this channel is alive or not.
   */
  bool Alive() const { return mAlive; };

  void Send(const unsigned char* src, std::size_t n) override;
  std::size_t Recv(unsigned char* dst, std::size_t n) override;
  bool HasData() override;
  void Close() override;

 private:
  bool mAlive;
  int mSocket;
};

}  // namespace scl

#endif  // SCL_NET_TCP_CHANNEL_H
