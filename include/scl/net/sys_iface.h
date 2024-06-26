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

#ifndef SCL_NET_SYS_IFACE_H
#define SCL_NET_SYS_IFACE_H

#include <cerrno>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace scl::net::details {

/**
 * @brief System call wrapper.
 *
 * SysIFace is a very simple wrapper for system calls. The primary purpose of
 * this class is to allow testing of code that depends on some system call.
 */
struct SysIFace {
  /**
   * @brief See man 3 errno.
   */
  static auto getError() {
    return errno;
  }

  /**
   * @brief See man 2 socket.
   */
  static auto socket(int domain, int type, int protocol) {
    return ::socket(domain, type, protocol);
  }

  /**
   * @brief See man 2 fcntl.
   */
  static auto fcntl(int fd, int cmd, int flags) {
    return ::fcntl(fd, cmd, flags);
  }

  /**
   * @brief See man 2 setsockopt.
   */
  static auto setSockOpt(int sockfd,
                         int level,
                         int optname,
                         const void* optval,
                         socklen_t optlen) {
    return ::setsockopt(sockfd, level, optname, optval, optlen);
  }

  /**
   * @brief See man htons.
   */
  static auto hostToNet(short hostshort) {
    return ::htons(hostshort);
  }

  /**
   * @brief See man 2 bind.
   */
  static auto bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    return ::bind(sockfd, addr, addrlen);
  }

  /**
   * @brief See man 2 listen.
   */
  static auto listen(int sockfd, int backlog) {
    return ::listen(sockfd, backlog);
  }

  /**
   * @brief See man 2 accept.
   */
  static auto accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    return ::accept(sockfd, addr, addrlen);
  }

  /**
   * @brief See man 3 inet_pton.
   */
  static auto addrToBin(int af, const char* src, void* dst) {
    return ::inet_pton(af, src, dst);
  }

  /**
   * @brief See man 3 inet_ntoa.
   */
  static auto netToAddr(struct in_addr inp) {
    return ::inet_ntoa(inp);
  }

  /**
   * @brief See man 2 connect.
   */
  static auto connect(int sockfd,
                      const struct sockaddr* addr,
                      socklen_t addrlen) {
    return ::connect(sockfd, addr, addrlen);
  }

  /**
   * @brief See man 2 poll.
   */
  static auto poll(struct pollfd* fds, nfds_t nfds, int timeout) {
    return ::poll(fds, nfds, timeout);
  }

  /**
   * @brief See man 2 close.
   */
  static auto close(int fd) {
    return ::close(fd);
  }

  /**
   * @brief See man 2 read.
   */
  static auto read(int fd, void* buf, size_t count) {
    return ::read(fd, buf, count);
  }

  /**
   * @brief See man 2 write.
   */
  static auto write(int fd, const void* buf, size_t count) {
    return ::write(fd, buf, count);
  }
};

}  // namespace scl::net::details

#endif  // SCL_NET_SYS_IFACE_H
