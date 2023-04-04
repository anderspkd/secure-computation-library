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

#ifndef SCL_NET_TCP_UTILS_H
#define SCL_NET_TCP_UTILS_H

#include <memory>
#include <stdexcept>
#include <system_error>
#include <thread>

#include <netinet/in.h>
#include <sys/socket.h>

#include "scl/net/sys_iface.h"

namespace scl::net {

/**
 * @brief Socket type. Probably <code>int</code>.
 */
using SocketType = int;

/**
 * @brief A connection.
 */
struct Connection {
  /**
   * @brief The socket.
   */
  SocketType socket;

  /**
   * @brief The hostname of the remote peer.
   */
  std::string hostname;
};

/**
 * @brief Create a socket listening on a port.
 * @tparam Sys interface for system calls
 * @param port the port to listen on
 * @param backlog the number of connections to accept
 * @return A socket.
 */
template <typename Sys = SysIFace>
SocketType CreateServerSocket(int port, int backlog) {
  SocketType ssock = Sys::Socket(AF_INET, SOCK_STREAM, 0);

  if (ssock < 0) {
    throw std::system_error(Sys::GetError(),
                            std::generic_category(),
                            "could not acquire server socket");
  }

  int opt = 1;
  auto options = SO_REUSEADDR | SO_REUSEPORT;

  if (Sys::SetSockOpt(ssock, SOL_SOCKET, options, &opt, sizeof(opt)) < 0) {
    throw std::system_error(Sys::GetError(),
                            std::generic_category(),
                            "could not set socket options");
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = Sys::HostToNet(INADDR_ANY);
  addr.sin_port = Sys::HostToNet(port);

  struct sockaddr* addr_ptr = (struct sockaddr*)&addr;

  if (Sys::Bind(ssock, addr_ptr, sizeof(addr)) < 0) {
    throw std::system_error(Sys::GetError(),
                            std::generic_category(),
                            "could not bind socket");
  }

  if (Sys::Listen(ssock, backlog)) {
    throw std::system_error(Sys::GetError(),
                            std::generic_category(),
                            "could not listen on socket");
  }

  return ssock;
}

/**
 * @brief Accept a connection.
 * @tparam Sys interface for system calls
 * @param server_socket a socket obtained from CreateServerSocket
 * @return An accepted connection
 */
template <typename Sys = SysIFace>
Connection AcceptConnection(SocketType server_socket) {
  auto sa = std::make_unique<struct sockaddr>();
  auto addrsize = sizeof(struct sockaddr_in);
  SocketType sock = Sys::Accept(server_socket, sa.get(), (socklen_t*)&addrsize);

  if (sock < 0) {
    throw std::system_error(Sys::GetError(),
                            std::generic_category(),
                            "could not accept connection");
  }

  const auto* p = (struct sockaddr_in*)sa.get();
  std::string hostname = Sys::NetToAddr(p->sin_addr);

  return {sock, hostname};
}

/**
 * @brief Connect to a remote host as a client.
 * @tparam Sys interface for system calls
 * @param hostname the hostname of the remote peer
 * @param port the port of the remote peer
 * @return A socket.
 */
template <typename Sys = SysIFace>
SocketType ConnectAsClient(const std::string& hostname, int port) {
  using namespace std::chrono_literals;

  SocketType sock = Sys::Socket(AF_INET, SOCK_STREAM, 0);

  if (sock < 0) {
    throw std::system_error(Sys::GetError(),
                            std::generic_category(),
                            "could not acquire socket");
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = Sys::HostToNet(port);

  int err = Sys::AddrToBin(AF_INET, hostname.c_str(), &(addr.sin_addr));

  if (err == 0) {
    throw std::runtime_error("invalid hostname");
  }

  while (Sys::Connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    std::this_thread::sleep_for(300ms);
  }

  return sock;
}

}  // namespace scl::net

#endif  // SCL_NET_TCP_UTILS_H
