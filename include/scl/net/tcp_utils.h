/**
 * @file tcp_utils.h
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

#ifndef _SCL_NET_TCP_UTILS_H
#define _SCL_NET_TCP_UTILS_H

#include <sys/socket.h>

#include <memory>
#include <system_error>

namespace scl {
namespace details {

/**
 * @brief Throw an error message.
 */
inline void ThrowError(const char* errmsg) {
  throw std::system_error(errno, std::generic_category(), errmsg);
}

/**
 * @brief Create a socket that listens on some port.
 * @param port the port to listen on
 * @param backlog the number of connections to listen for
 */
int CreateServerSocket(int port, int backlog);

/**
 * @brief Information about an accepted connection.
 */
struct AcceptedConnection {
  /**
   * @brief The socket.
   */
  int socket;

  /**
   * @brief Information returned as part of accept(2).
   */
  struct std::shared_ptr<sockaddr> socket_info;
};

/**
 * @brief Accept a connection.
 * @param server_socket a socket that is listening on connections
 * @return an accepted connection.
 */
AcceptedConnection AcceptConnection(int server_socket);

/**
 * @brief Extra the hostname of an accepted connection.
 */
std::string GetAddress(AcceptedConnection connection);

/**
 * @brief Connect in client mode.
 * @param hostname the hostname of the server
 * @param port the port of the server
 * @return a socket.
 */
int ConnectAsClient(std::string hostname, int port);

/**
 * @brief Close a socket.
 */
int CloseSocket(int socket);

/**
 * @brief Read from a socket.
 */
int ReadFromSocket(int socket, unsigned char* dst, std::size_t n);

/**
 * @brief Write to a socket.
 */
int WriteToSocket(int socket, const unsigned char* src, std::size_t n);

}  // namespace details
}  // namespace scl

#endif  // _SCL_NET_TCP_UTILS_H
