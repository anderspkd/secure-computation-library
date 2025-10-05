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

#include "scl/net/tcp/channel.h"

#include "./syscalls.h"
#include "./utils.h"
#include "scl/coro/runtime.h"

using namespace scl;

void TcpChannel::close() {
  if (m_alive) {
    // ensures that we only attempt to close the socket once, even if closing
    // the somehow socket fails.
    m_alive = false;

    if (details::sys_call::close(m_socket) < 0) {
      throw std::system_error(details::sys_call::getError(),
                              std::generic_category(),
                              "close failed");
    }
  }
}

Task<void> TcpChannel::send(Packet&& packet) {
  co_await send(packet);
}

Task<void> TcpChannel::send(const Packet& packet) {
  // Write the packet size to a buffer.
  const Packet::SizeType pkt_sz = packet.dataSize();
  const auto pkt_t_sz = sizeof(Packet::SizeType);
  unsigned char packet_size_buf[pkt_t_sz] = {0};
  std::memcpy(packet_size_buf, &pkt_sz, pkt_t_sz);

  // assume writing the packet size won't block. It probably wont.
  if (details::sys_call::write(m_socket, packet_size_buf, pkt_t_sz) < 0) {
    throw std::system_error(details::sys_call::getError(),
                            std::generic_category(),
                            "writing packet size failed");
  }

  // Write content of packet. This may block, in which case a RetrySend
  // awaitable is created and this coroutine is suspended.
  std::size_t rem = packet.dataSize();
  const unsigned char* data = packet.get();
  while (rem > 0) {
    const auto written = details::sys_call::write(m_socket, data, rem);
    if (written < 0) {
      const auto err = details::sys_call::getError();
      if (err == EAGAIN || err == EWOULDBLOCK) {
        co_await [socket = m_socket]() {
          return details::pollSocket(socket, POLLOUT);
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

namespace {

Task<void> recvInto(int socket, unsigned char* dst, std::size_t nbytes) {
  std::size_t rem = nbytes;

  while (rem > 0) {
    const auto read = details::sys_call::read(socket, dst, rem);
    if (read < 0) {
      const auto err = details::sys_call::getError();

      if (err == EAGAIN || err == EWOULDBLOCK) {
        co_await [socket]() { return details::pollSocket(socket, POLLIN); };
      } else {
        throw std::system_error(err, std::generic_category(), "recv failed");
      }

    } else {
      rem -= read;
      dst += read;
    }
  }
}

Task<bool> recvInto(int socket,
                    unsigned char* dst,
                    std::size_t nbytes,
                    Time::Duration timeout) {
  const auto start = Time::now();
  bool ready = false;

  co_await [&ready, timeout, start, socket]() {
    if (Time::now() - start >= timeout) {
      return true;
    } else {
      ready = details::pollSocket(socket, POLLIN);
      return ready;
    }
  };

  if (ready) {
    co_await recvInto(socket, dst, nbytes);
    co_return true;
  }

  co_return false;
}

}  // namespace

Task<Packet> TcpChannel::recv() {
  unsigned char packet_size_buf[sizeof(Packet::SizeType)] = {0};

  // read size of the packet.
  co_await recvInto(m_socket, packet_size_buf, sizeof(Packet::SizeType));
  Packet::SizeType packet_size;
  std::memcpy(&packet_size, packet_size_buf, sizeof(Packet::SizeType));

  Packet packet(packet_size);
  co_await recvInto(m_socket, packet.get(), packet_size);
  packet.setWritePtr(packet_size);

  co_return packet;
}

Task<std::optional<Packet>> TcpChannel::recv(Time::Duration timeout) {
  unsigned char packet_size_buf[sizeof(Packet::SizeType)] = {0};

  // attempt to read the size of the packet. If we timeout here, we will simply
  // skip the rest of the function.
  bool timed_out = co_await recvInto(m_socket,
                                     packet_size_buf,
                                     sizeof(Packet::SizeType),
                                     timeout);

  // exit early if we timed out
  if (timed_out) {
    co_return {};
  }

  // otherwise behave as normal recv.
  Packet::SizeType packet_size;
  std::memcpy(&packet_size, packet_size_buf, sizeof(Packet::SizeType));

  Packet packet(packet_size);
  co_await recvInto(m_socket, packet.get(), packet_size);
  packet.setWritePtr(packet_size);

  co_return packet;
}

Task<bool> TcpChannel::poll() {
  co_return details::pollSocket(m_socket, POLLIN);
}
