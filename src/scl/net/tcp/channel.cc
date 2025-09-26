#include "./syscalls.h"
#include "./utils.h"
#include "scl/coro/runtime.h"
#include "scl/net/tcp/channel.h"

void scl::TcpChannel::close() {
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

scl::Task<void> scl::TcpChannel::send(Packet&& packet) {
  co_await send(packet);
}

scl::Task<void> scl::TcpChannel::send(const Packet& packet) {
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

scl::Task<void> recvInto(int socket, unsigned char* dst, std::size_t nbytes) {
  std::size_t rem = nbytes;
  while (rem > 0) {
    const auto read = scl::details::sys_call::read(socket, dst, rem);
    if (read < 0) {
      const auto err = scl::details::sys_call::getError();
      if (err == EAGAIN || err == EWOULDBLOCK) {
        co_await [socket = socket]() {
          return scl::details::pollSocket(socket, POLLIN);
        };
      } else {
        throw std::system_error(err, std::generic_category(), "recv failed");
      }
    } else {
      rem -= read;
      dst += read;
    }
  }
}

}  // namespace

scl::Task<scl::Packet> scl::TcpChannel::recv() {
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

scl::Task<bool> scl::TcpChannel::poll() {
  co_return details::pollSocket(m_socket, POLLIN);
}
