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

#include "./utils.h"

#include "./syscalls.h"

int scl::details::createServerSocket(int port, int backlog) {
  int ssock = sys_call::socket(AF_INET, SOCK_STREAM, 0);

  if (ssock < 0) {
    throw std::system_error(sys_call::getError(),
                            std::generic_category(),
                            "could not acquire server socket");
  }

  int opt = 1;
  auto options = SO_REUSEADDR | SO_REUSEPORT;

  if (sys_call::setSockOpt(ssock, SOL_SOCKET, options, &opt, sizeof(opt)) < 0) {
    throw std::system_error(sys_call::getError(),
                            std::generic_category(),
                            "could not set socket options");
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = sys_call::hostToNet(INADDR_ANY);
  addr.sin_port = sys_call::hostToNet(port);

  struct sockaddr* addr_ptr = (struct sockaddr*)&addr;

  if (bind(ssock, addr_ptr, sizeof(addr)) < 0) {
    throw std::system_error(sys_call::getError(),
                            std::generic_category(),
                            "could not bind socket");
  }

  if (listen(ssock, backlog)) {
    throw std::system_error(sys_call::getError(),
                            std::generic_category(),
                            "could not listen on socket");
  }

  return ssock;
}

scl::details::Connection scl::details::acceptConnection(int server_socket) {
  struct sockaddr sa;
  auto addrsize = sizeof(struct sockaddr_in);
  int sock = sys_call::accept(server_socket, &sa, (socklen_t*)&addrsize);

  if (sock < 0) {
    throw std::system_error(sys_call::getError(),
                            std::generic_category(),
                            "could not accept connection");
  }

  const auto* p = (struct sockaddr_in*)&sa;
  std::string hostname = sys_call::netToAddr(p->sin_addr);

  return {sock, hostname};
}

int scl::details::connectAsClient(const std::string& hostname, int port) {
  int sock = sys_call::socket(AF_INET, SOCK_STREAM, 0);

  if (sock < 0) {
    throw std::system_error(sys_call::getError(),
                            std::generic_category(),
                            "could not acquire socket");
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = sys_call::hostToNet(port);

  int err = sys_call::addrToBin(AF_INET, hostname.c_str(), &(addr.sin_addr));

  if (err == 0) {
    throw std::runtime_error("invalid hostname");
  }

  if (sys_call::connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    throw std::system_error(sys_call::getError(),
                            std::generic_category(),
                            "could not connect");
  }

  return sock;
}

void scl::details::markSocketNonBlocking(int socket) {
  auto flags = sys_call::fcntl(socket, F_GETFL, 0);
  if (flags == -1) {
    throw std::system_error(sys_call::getError(),
                            std::generic_category(),
                            "could not read current flags of socket");
  }

  if (sys_call::fcntl(socket, F_SETFL, flags | O_NONBLOCK) == -1) {
    throw std::system_error(sys_call::getError(),
                            std::generic_category(),
                            "could not set O_NONBLOCK on socket");
  }
}

bool scl::details::pollSocket(int socket, short event) {
  struct pollfd fds{socket, POLLIN, 0};

  auto r = sys_call::poll(&fds, 1, 0);

  if (r < 0) {
    throw std::system_error(sys_call::getError(),
                            std::generic_category(),
                            "poll failed");
  }

  return r > 0 && fds.revents == event;
}
