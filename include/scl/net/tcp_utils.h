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

#ifndef SCL_NET_TCP_UTILS_H
#define SCL_NET_TCP_UTILS_H

#include <iostream>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <thread>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "scl/net/sys_iface.h"

namespace scl::net {

/**
 * @brief Socket type. Probably <code>int</code>.
 */
using SocketType = int;

namespace details {

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
SocketType createServerSocket(int port, int backlog) {
  SocketType ssock = Sys::socket(AF_INET, SOCK_STREAM, 0);

  if (ssock < 0) {
    throw std::system_error(Sys::getError(),
                            std::generic_category(),
                            "could not acquire server socket");
  }

  int opt = 1;
  auto options = SO_REUSEADDR | SO_REUSEPORT;

  if (Sys::setSockOpt(ssock, SOL_SOCKET, options, &opt, sizeof(opt)) < 0) {
    throw std::system_error(Sys::getError(),
                            std::generic_category(),
                            "could not set socket options");
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = Sys::hostToNet(INADDR_ANY);
  addr.sin_port = Sys::hostToNet(port);

  struct sockaddr* addr_ptr = (struct sockaddr*)&addr;

  if (Sys::bind(ssock, addr_ptr, sizeof(addr)) < 0) {
    throw std::system_error(Sys::getError(),
                            std::generic_category(),
                            "could not bind socket");
  }

  if (Sys::listen(ssock, backlog)) {
    throw std::system_error(Sys::getError(),
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
Connection acceptConnection(SocketType server_socket) {
  struct sockaddr sa;
  auto addrsize = sizeof(struct sockaddr_in);
  SocketType sock = Sys::accept(server_socket, &sa, (socklen_t*)&addrsize);

  if (sock < 0) {
    throw std::system_error(Sys::getError(),
                            std::generic_category(),
                            "could not accept connection");
  }

  const auto* p = (struct sockaddr_in*)&sa;
  std::string hostname = Sys::netToAddr(p->sin_addr);

  return {sock, hostname};
}

/**
 * @brief Connect to a remote host as a client.
 * @tparam Sys interface for system calls
 * @param hostname the hostname of the remote peer
 * @param port the port of the remote peer
 * @return A socket.
 */
template <typename SYS = SysIFace>
SocketType connectAsClient(const std::string& hostname, int port) {
  using namespace std::chrono_literals;

  SocketType sock = SYS::socket(AF_INET, SOCK_STREAM, 0);

  if (sock < 0) {
    throw std::system_error(SYS::getError(),
                            std::generic_category(),
                            "could not acquire socket");
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = SYS::hostToNet(port);

  int err = SYS::addrToBin(AF_INET, hostname.c_str(), &(addr.sin_addr));

  if (err == 0) {
    throw std::runtime_error("invalid hostname");
  }

  if (SYS::connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    throw std::system_error(SYS::getError(),
                            std::generic_category(),
                            "could not connect");
  }

  return sock;
}

template <typename SYS = SysIFace>
void markSocketNonBlocking(SocketType socket) {
  auto flags = SYS::fcntl(socket, F_GETFL, 0);
  if (flags == -1) {
    throw std::system_error(SYS::getError(),
                            std::generic_category(),
                            "could not read current flags of socket");
  }

  if (SYS::fcntl(socket, F_SETFL, flags | O_NONBLOCK) == -1) {
    throw std::system_error(SYS::getError(),
                            std::generic_category(),
                            "could not set O_NONBLOCK on socket");
  }
}

template <typename SYS = SysIFace>
bool pollSocket(SocketType socket, short event) {
  struct pollfd fds {
    socket, POLLIN, 0
  };

  auto r = SYS::poll(&fds, 1, 0);

  if (r < 0) {
    throw std::system_error(SYS::getError(),
                            std::generic_category(),
                            "poll failed");
  }

  return r > 0 && fds.revents == event;
}

}  // namespace details
}  // namespace scl::net

#endif  // SCL_NET_TCP_UTILS_H
