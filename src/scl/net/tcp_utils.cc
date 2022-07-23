/**
 * @file tcp_utils.cc
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

#include "scl/net/tcp_utils.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <system_error>
#include <thread>

int scl::details::CreateServerSocket(int port, int backlog) {
  int err;
  int ssock = ::socket(AF_INET, SOCK_STREAM, 0);
  if (ssock < 0) {
    SCL_THROW_SYS_ERROR("could not acquire server socket");
  }

  int opt = 1;
  auto options = SO_REUSEADDR | SO_REUSEPORT;
  err = ::setsockopt(ssock, SOL_SOCKET, options, &opt, sizeof(opt));
  if (err < 0) {
    SCL_THROW_SYS_ERROR("could not set socket options");
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ::htons(INADDR_ANY);
  addr.sin_port = ::htons(port);

  struct sockaddr* addr_ptr = (struct sockaddr*)&addr;

  err = ::bind(ssock, addr_ptr, sizeof(addr));
  if (err < 0) {
    SCL_THROW_SYS_ERROR("could not bind socket");
  }

  err = ::listen(ssock, backlog);
  if (err < 0) {
    SCL_THROW_SYS_ERROR("could not listen on socket");
  }

  return ssock;
}

scl::details::AcceptedConnection scl::details::AcceptConnection(
    int server_socket) {
  scl::details::AcceptedConnection ac;
  ac.socket_info = std::make_shared<struct sockaddr>();
  struct sockaddr_in addr;
  auto addrsize = sizeof(addr);
  ac.socket =
      ::accept(server_socket, ac.socket_info.get(), (socklen_t*)&addrsize);
  if (ac.socket < 0) {
    SCL_THROW_SYS_ERROR("could not accept connection");
  } else {
    return ac;
  }
}

std::string scl::details::GetAddress(
    scl::details::AcceptedConnection connection) {
  struct sockaddr_in* s =
      reinterpret_cast<struct sockaddr_in*>(connection.socket_info.get());
  return inet_ntoa(s->sin_addr);
}

int scl::details::ConnectAsClient(std::string hostname, int port) {
  using namespace std::chrono_literals;

  int sock = ::socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    SCL_THROW_SYS_ERROR("could not acquire socket");
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = ::htons(port);

  int err = ::inet_pton(AF_INET, hostname.c_str(), &(addr.sin_addr));
  if (err == 0) {
    throw std::runtime_error("invalid hostname");
  } else if (err < 0) {
    SCL_THROW_SYS_ERROR("invalid address family");
  }

  while (::connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    std::this_thread::sleep_for(300ms);

  return sock;
}

int scl::details::CloseSocket(int socket) { return ::close(socket); }

int scl::details::ReadFromSocket(int socket, unsigned char* dst,
                                 std::size_t n) {
  return ::read(socket, dst, n);
}

int scl::details::WriteToSocket(int socket, const unsigned char* src,
                                std::size_t n) {
  return ::write(socket, src, n);
}
