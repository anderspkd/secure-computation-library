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

#ifndef SCL_NET_TCP_UTILS_H
#define SCL_NET_TCP_UTILS_H

#include <string>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>

namespace scl::details {

struct Connection {
  int socket;
  std::string hostname;
};

int createServerSocket(int port, int backlog);
Connection acceptConnection(int server_socket);
int connectAsClient(const std::string& hostname, int port);
void markSocketNonBlocking(int socket);
bool pollSocket(int socket, short event);

}  // namespace scl::details

#endif  // SCL_NET_TCP_UTILS_H
