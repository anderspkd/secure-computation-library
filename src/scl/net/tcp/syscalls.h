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

#ifndef SCL_NET_TCP_SYSCALLS_H
#define SCL_NET_TCP_SYSCALLS_H

#include <cerrno>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace scl::details::sys_call {

inline auto getError() {
  return errno;
}

inline auto socket(int domain, int type, int protocol) {
  return ::socket(domain, type, protocol);
}

inline auto fcntl(int fd, int cmd, int flags) {
  return ::fcntl(fd, cmd, flags);
}

inline auto setSockOpt(int sockfd,
                       int level,
                       int optname,
                       const void* optval,
                       socklen_t optlen) {
  return ::setsockopt(sockfd, level, optname, optval, optlen);
}

inline auto hostToNet(short hostshort) {
  return ::htons(hostshort);
}

inline auto bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
  return ::bind(sockfd, addr, addrlen);
}

inline auto listen(int sockfd, int backlog) {
  return ::listen(sockfd, backlog);
}

inline auto accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
  return ::accept(sockfd, addr, addrlen);
}

inline auto addrToBin(int af, const char* src, void* dst) {
  return ::inet_pton(af, src, dst);
}

inline auto netToAddr(struct in_addr inp) {
  return ::inet_ntoa(inp);
}

inline auto connect(int sockfd,
                    const struct sockaddr* addr,
                    socklen_t addrlen) {
  return ::connect(sockfd, addr, addrlen);
}

inline auto poll(struct pollfd* fds, nfds_t nfds, int timeout) {
  return ::poll(fds, nfds, timeout);
}

inline auto close(int sockfd) {
  return ::close(sockfd);
}

inline auto read(int sockfd, void* buf, size_t count) {
  return ::read(sockfd, buf, count);
}

inline auto write(int sockfd, const void* buf, size_t count) {
  return ::write(sockfd, buf, count);
}

}  // namespace scl::details::sys_call

#endif  // SCL_NET_TCP_SYSCALLS_H
