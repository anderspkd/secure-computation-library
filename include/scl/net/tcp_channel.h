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

#ifndef SCL_NET_TCP_CHANNEL_H
#define SCL_NET_TCP_CHANNEL_H

#include <cstring>
#include <system_error>

#include <sys/poll.h>

#include "scl/coro/future.h"
#include "scl/net/channel.h"
#include "scl/net/config.h"
#include "scl/net/sys_iface.h"
#include "scl/net/tcp_utils.h"

namespace scl::net {

/**
 * @brief A channel implementation using TCP.
 */
template <typename SYS = details::SysIFace>
class TcpChannel final : public Channel {
 public:
  /**
   * @brief Create a new TcpChannel.
   * @param socket the socket.
   */
  TcpChannel(SocketType socket) : m_alive(true), m_socket(socket) {}

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
  coro::Task<void> send(Packet&& packet) override;

  /**
   * @brief Send a packet on this channel.
   * @param packet the packet to send.
   *
   * This function will attempt to send \p packet on the underlying socket. If
   * this would result in the call blocking, then the function is suspended and
   * scheduled to run later through the supplied scheduler.
   */
  coro::Task<void> send(const Packet& packet) override;

  /**
   * @brief Recv a packet on this channel.
   * @return the received packet.
   *
   * This function will suspend execution if not enough data is ready yet. To
   * check if it's possible to receive something on the channel, use hasData().
   */
  coro::Task<Packet> recv() override;

  /**
   * @brief Check if this channel has data ready for recovering.
   * @return true if there's data to receive and false otherwise.
   */
  coro::Task<bool> hasData() override;

 private:
  bool m_alive;
  SocketType m_socket;
};

template <typename SYS>
void TcpChannel<SYS>::close() {
  if (m_alive) {
    // ensures that we only attempt to close the socket once, even if closing
    // the somehow socket fails.
    m_alive = false;

    if (SYS::close(m_socket) < 0) {
      throw std::system_error(SYS::getError(),
                              std::generic_category(),
                              "close failed");
    }
  }
}

template <typename SYS>
coro::Task<void> TcpChannel<SYS>::send(Packet&& packet) {
  co_await send(packet);
}

template <typename SYS>
coro::Task<void> TcpChannel<SYS>::send(const Packet& packet) {
  // Write the packet size to a buffer.
  const Packet::SizeType packet_size = packet.size();
  const auto packet_size_size = sizeof(Packet::SizeType);
  unsigned char packet_size_buf[packet_size_size] = {0};
  std::memcpy(packet_size_buf, &packet_size, packet_size_size);

  // assume writing the packet size won't block. It probably wont.
  if (SYS::write(m_socket, packet_size_buf, packet_size_size) < 0) {
    throw std::system_error(SYS::getError(),
                            std::generic_category(),
                            "writing packet size failed");
  }

  // Write content of packet. This may block, in which case a RetrySend
  // awaitable is created and this coroutine is suspended.
  std::size_t rem = packet.size();
  const unsigned char* data = packet.get();
  while (rem > 0) {
    const auto written = SYS::write(m_socket, data, rem);
    if (written < 0) {
      const auto err = SYS::getError();
      if (err == EAGAIN || err == EWOULDBLOCK) {
        co_await [socket = m_socket]() {
          return details::pollSocket<SYS>(socket, POLLOUT);
        };
      } else {
        throw std::system_error(err, std::generic_category(), "send failed");
      }
    } else {
      rem -= written;
      data += written;
    }
  }
}

namespace details {

// Helper coroutine for reading some amount of bytes from a socket into a
// buffer. If the read would block, then the call is suspended using the
// provided scheduler.
template <typename SYS>
coro::Task<void> recvInto(SocketType socket,
                          unsigned char* dst,
                          std::size_t nbytes) {
  std::size_t rem = nbytes;
  while (rem > 0) {
    const auto read = SYS::read(socket, dst, rem);
    if (read < 0) {
      const auto err = SYS::getError();
      if (err == EAGAIN || err == EWOULDBLOCK) {
        co_await
            [socket = socket]() { return pollSocket<SYS>(socket, POLLIN); };
      } else {
        throw std::system_error(err, std::generic_category(), "recv failed");
      }
    } else {
      rem -= read;
      dst += read;
    }
  }
}

}  // namespace details

template <typename SYS>
coro::Task<Packet> TcpChannel<SYS>::recv() {
  unsigned char packet_size_buf[sizeof(Packet::SizeType)] = {0};

  // read size of the packet.
  co_await details::recvInto<SYS>(m_socket,
                                  packet_size_buf,
                                  sizeof(Packet::SizeType));
  Packet::SizeType packet_size;
  std::memcpy(&packet_size, packet_size_buf, sizeof(Packet::SizeType));

  Packet packet(packet_size);
  co_await details::recvInto<SYS>(m_socket, packet.get(), packet_size);
  packet.setWritePtr(packet_size);

  co_return packet;
}

template <typename SYS>
coro::Task<bool> TcpChannel<SYS>::hasData() {
  co_return details::pollSocket(m_socket, POLLIN);
}

}  // namespace scl::net

#endif  // SCL_NET_TCP_CHANNEL_H
