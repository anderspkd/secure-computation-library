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

#include <catch2/catch.hpp>
#include <stdexcept>
#include <system_error>

#include "scl/net/sys_iface.h"
#include "scl/net/tcp_utils.h"
#include "util.h"

using namespace scl;

TEST_CASE("SysIFace GetError", "[net]") {
  // This is needed to make the test stable when computing coverage. Not sure
  // why.
  errno = 0;
  REQUIRE(net::SysIFace::GetError() == 0);
}

#define DEFAULT_SETSOCKOPT                                                    \
  static auto SetSockOpt(int s, int l, int o, const void* ov, socklen_t ol) { \
    return net::SysIFace::SetSockOpt(s, l, o, ov, ol);                        \
  }

#define DEFAULT_HOST_TO_NET             \
  static auto HostToNet(short h) {      \
    return net::SysIFace::HostToNet(h); \
  }

#define DEFAULT_BIND                                                \
  static auto Bind(int s, const struct sockaddr* a, socklen_t al) { \
    return net::SysIFace::Bind(s, a, al);                           \
  }

#define DEFAULT_LISTEN                  \
  static auto Listen(int s, int b) {    \
    return net::SysIFace::Listen(s, b); \
  }

#define DEFAULT_ADDR_TO_BIN                              \
  static auto AddrToBin(int a, const char* s, void* d) { \
    return net::SysIFace::AddrToBin(a, s, d);            \
  }

#define DEFAULT_CONNECT                                                \
  static auto Connect(int s, const struct sockaddr* a, socklen_t al) { \
    return net::SysIFace::Connect(s, a, al);                           \
  }

struct SysIFace_SocketFails {
  DEFAULT_BIND;
  DEFAULT_HOST_TO_NET;
  DEFAULT_LISTEN;
  DEFAULT_SETSOCKOPT;
  DEFAULT_ADDR_TO_BIN;
  DEFAULT_CONNECT;

  static auto GetError() {
    return EACCES;
  }

  static auto Socket(int domain, int type, int protocol) {
    (void)domain;
    (void)type;
    (void)protocol;
    return -1;
  }
};

TEST_CASE("CreateServerSocket fails on socket", "[net]") {
  REQUIRE_THROWS_MATCHES(
      net::CreateServerSocket<SysIFace_SocketFails>(1, 1),
      std::system_error,
      Catch::Matchers::Message(
          "could not acquire server socket: Permission denied"));
}

#define DEFAULT_SOCKET                      \
  static auto Socket(int d, int t, int p) { \
    return net::SysIFace::Socket(d, t, p);  \
  }

struct SysIFace_SetSockOptFails {
  DEFAULT_BIND;
  DEFAULT_HOST_TO_NET;
  DEFAULT_LISTEN;
  DEFAULT_SOCKET;

  static auto GetError() {
    return EBADF;
  }

  static auto SetSockOpt(int sockfd,
                         int level,
                         int optname,
                         const void* optval,
                         socklen_t optlen) {
    (void)sockfd;
    (void)level;
    (void)optname;
    (void)optval;
    (void)optlen;
    return -1;
  }
};

TEST_CASE("CreateServerSocket fails on setsockopt", "[net]") {
  REQUIRE_THROWS_MATCHES(
      net::CreateServerSocket<SysIFace_SetSockOptFails>(1, 1),
      std::system_error,
      Catch::Matchers::Message(
          "could not set socket options: Bad file descriptor"));
}

struct SysIFace_BindFails {
  DEFAULT_HOST_TO_NET;
  DEFAULT_LISTEN;
  DEFAULT_SETSOCKOPT;
  DEFAULT_SOCKET;

  static auto GetError() {
    return EACCES;
  }

  static auto Bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    (void)sockfd;
    (void)addr;
    (void)addrlen;
    return -1;
  }
};

TEST_CASE("CreateServerSocket fails on bind", "[net]") {
  REQUIRE_THROWS_MATCHES(
      net::CreateServerSocket<SysIFace_BindFails>(1, 1),
      std::system_error,
      Catch::Matchers::Message("could not bind socket: Permission denied"));
}

struct SysIFace_ListenFails {
  DEFAULT_HOST_TO_NET;
  DEFAULT_SOCKET;
  DEFAULT_SETSOCKOPT;
  DEFAULT_BIND;

  static auto GetError() {
    return EADDRINUSE;
  }

  static auto Listen(int sockfd, int backlog) {
    (void)sockfd;
    (void)backlog;
    return -1;
  }
};

TEST_CASE("CreateServerSocket fails on listen", "[net]") {
  const auto port = test::GetPort();
  REQUIRE_THROWS_MATCHES(
      net::CreateServerSocket<SysIFace_ListenFails>(port, 1),
      std::system_error,
      Catch::Matchers::Message(
          "could not listen on socket: Address already in use"));
}

struct SysIFace_AcceptFails {
  static auto NetToAddr(struct in_addr inp) {
    return net::SysIFace::NetToAddr(inp);
  }

  static auto GetError() {
    return EAGAIN;
  }

  static auto Accept(int sockfd,
                     struct sockaddr* addr,
                     const socklen_t* addrlen) {
    (void)sockfd;
    (void)addr;
    (void)addrlen;
    return -1;
  }
};

TEST_CASE("AcceptConnection fails on accept", "[net]") {
  REQUIRE_THROWS_MATCHES(
      net::AcceptConnection<SysIFace_AcceptFails>(0),
      std::system_error,
      Catch::Matchers::Message(
          "could not accept connection: Resource temporarily unavailable"));
}

TEST_CASE("ConnectAsClient fails on socket", "[net]") {
  REQUIRE_THROWS_MATCHES(
      net::ConnectAsClient<SysIFace_SocketFails>("127.0.0.1", 1111),
      std::system_error,
      Catch::Matchers::Message("could not acquire socket: Permission denied"));
}

TEST_CASE("ConnectAsClient invalid address") {
  REQUIRE_THROWS_MATCHES(
      net::ConnectAsClient<net::SysIFace>("not a valid hostname", 1111),
      std::runtime_error,
      Catch::Matchers::Message("invalid hostname"));
}
