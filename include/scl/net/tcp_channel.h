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

#ifndef SCL_NET_TCP_CHANNEL_H
#define SCL_NET_TCP_CHANNEL_H

#include <cstring>
#include <memory>
#include <vector>

#include <sys/poll.h>

#include "scl/net/channel.h"
#include "scl/net/config.h"
#include "scl/net/sys_iface.h"

namespace scl::net {

/**
 * @brief A channel between two peers utilizing TCP.
 */
template <typename Sys = SysIFace>
class TcpChannel final : public Channel {
 public:
  /**
   * @brief Wrap a socket in a TCP channel.
   * @param socket the socket.
   */
  TcpChannel(int socket) : mAlive(true), mSocket(socket){};

  /**
   * @brief Tells whether this channel is alive or not.
   */
  bool Alive() const {
    return mAlive;
  };

  void Send(const unsigned char* src, std::size_t n) override;
  std::size_t Recv(unsigned char* dst, std::size_t n) override;
  bool HasData() override;
  void Close() override;

 private:
  bool mAlive;
  int mSocket;
};

template <typename Sys>
void TcpChannel<Sys>::Send(const unsigned char* src, std::size_t n) {
  std::size_t rem = n;
  std::size_t offset = 0;

  while (rem > 0) {
    auto sent = Sys::Write(mSocket, src + offset, rem);

    if (sent < 0) {
      throw std::system_error(Sys::GetError(),
                              std::generic_category(),
                              "write failed");
    }

    rem -= sent;
    offset += sent;
  }
}

template <typename Sys>
std::size_t TcpChannel<Sys>::Recv(unsigned char* dst, std::size_t n) {
  std::size_t rem = n;
  std::size_t offset = 0;

  while (rem > 0) {
    auto recv = Sys::Read(mSocket, dst + offset, rem);

    if (recv == 0) {
      break;
    }

    if (recv < 0) {
      throw std::system_error(Sys::GetError(),
                              std::generic_category(),
                              "read failed");
    }

    rem -= recv;
    offset += recv;
  }

  return n - rem;
}

template <typename Sys>
bool TcpChannel<Sys>::HasData() {
  struct pollfd fds {
    mSocket, POLLIN, 0
  };

  auto r = Sys::Poll(&fds, 1, 0);

  if (r < 0) {
    throw std::system_error(Sys::GetError(),
                            std::generic_category(),
                            "poll failed");
  }

  return r > 0 && fds.revents == POLLIN;
}

template <typename Sys>
void TcpChannel<Sys>::Close() {
  if (!mAlive) {
    return;
  }

  mAlive = false;

  if (Sys::Close(mSocket) < 0) {
    throw std::system_error(Sys::GetError(),
                            std::generic_category(),
                            "close failed");
  }
}

}  // namespace scl::net

#endif  // SCL_NET_TCP_CHANNEL_H
