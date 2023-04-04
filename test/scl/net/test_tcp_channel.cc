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
#include <system_error>
#include <thread>

#include "scl/net/sys_iface.h"
#include "scl/net/tcp_channel.h"
#include "scl/net/tcp_utils.h"
#include "scl/util/prg.h"
#include "util.h"

using namespace scl;

TEST_CASE("TcpChannel connect and then close", "[net]") {
  auto port = test::GetPort();

  std::shared_ptr<net::TcpChannel<>> client;
  std::shared_ptr<net::TcpChannel<>> server;

  std::thread clt([&]() {
    int socket = net::ConnectAsClient("0.0.0.0", port);
    client = std::make_shared<net::TcpChannel<>>(socket);
  });

  std::thread srv([&]() {
    int ssock = net::CreateServerSocket(port, 1);
    auto ac = net::AcceptConnection(ssock);
    server = std::make_shared<net::TcpChannel<>>(ac.socket);
    net::SysIFace::Close(ssock);
  });

  clt.join();
  srv.join();

  REQUIRE(client->Alive());
  REQUIRE(server->Alive());

  client->Close();
  server->Close();

  REQUIRE(!server->Alive());
  REQUIRE(!client->Alive());
}

TEST_CASE("TcpChannel send/recv", "[net]") {
  auto port = scl::test::GetPort();

  std::shared_ptr<net::TcpChannel<>> client;
  std::shared_ptr<net::TcpChannel<>> server;

  std::thread clt([&]() {
    int socket = net::ConnectAsClient("0.0.0.0", port);
    client = std::make_shared<net::TcpChannel<>>(socket);
  });

  std::thread srv([&]() {
    int ssock = net::CreateServerSocket(port, 1);
    auto ac = net::AcceptConnection(ssock);
    server = std::make_shared<net::TcpChannel<>>(ac.socket);
    net::SysIFace::Close(ssock);
  });

  clt.join();
  srv.join();

  auto prg = util::PRG::Create();
  unsigned char send[200] = {0};
  unsigned char recv[200] = {0};
  prg.Next(send, 200);

  REQUIRE(!server->HasData());

  client->Send(send, 100);
  client->Send(send + 100, 100);

  REQUIRE(server->HasData());
  server->Recv(recv, 20);
  server->Recv(recv + 20, 180);

  REQUIRE(test::BufferEquals(send, recv, 200));
}

TEST_CASE("TcpChannel recv from closed socket", "[net]") {
  auto port = test::GetPort();

  std::shared_ptr<net::TcpChannel<>> client;
  std::shared_ptr<net::TcpChannel<>> server;

  std::thread clt([&]() {
    int socket = net::ConnectAsClient("0.0.0.0", port);
    client = std::make_shared<net::TcpChannel<>>(socket);
  });

  std::thread srv([&]() {
    int ssock = net::CreateServerSocket(port, 1);
    auto ac = net::AcceptConnection(ssock);
    server = std::make_shared<net::TcpChannel<>>(ac.socket);
    net::SysIFace::Close(ssock);
  });

  clt.join();
  srv.join();

  client->Close();
  unsigned char buf[3] = {0};
  auto r = server->Recv(buf, 3);
  REQUIRE(r == 0);
}

#define DEFAULT_READ                                  \
  static auto Read(int fd, void* buf, size_t count) { \
    return net::SysIFace::Read(fd, buf, count);       \
  }

#define DEFAULT_CLOSE                \
  static auto Close(int fd) {        \
    return net::SysIFace::Close(fd); \
  }

#define DEFAULT_POLL                                               \
  static auto Poll(struct pollfd* fds, nfds_t nfds, int timeout) { \
    return net::SysIFace::Poll(fds, nfds, timeout);                \
  }

#define DEFAULT_WRITE                                        \
  static auto Write(int fd, const void* buf, size_t count) { \
    return net::SysIFace::Write(fd, buf, count);             \
  }

struct SysIFace_WriteFails {
  DEFAULT_READ;
  DEFAULT_CLOSE;
  DEFAULT_POLL;

  static auto GetError() {
    return EAGAIN;
  }

  static int Write(int fd, const void* buf, size_t count) {
    (void)fd;
    (void)buf;
    (void)count;
    return -1;
  }
};

TEST_CASE("TcpChannel Send fails", "[net]") {
  net::TcpChannel<SysIFace_WriteFails> c(1);
  REQUIRE_THROWS_MATCHES(c.Send(nullptr, 1),
                         std::system_error,
                         Catch::Matchers::Message(
                             "write failed: Resource temporarily unavailable"));
}

struct SysIFace_ReadFails {
  DEFAULT_CLOSE;
  DEFAULT_POLL;
  DEFAULT_WRITE;

  static auto GetError() {
    return EAGAIN;
  }

  static int Read(int fd, void* buf, size_t count) {
    (void)fd;
    (void)buf;
    (void)count;
    return -1;
  }
};

TEST_CASE("TcpChannel Recv fails", "[net]") {
  net::TcpChannel<SysIFace_ReadFails> c(1);
  REQUIRE_THROWS_MATCHES(c.Recv(nullptr, 1),
                         std::system_error,
                         Catch::Matchers::Message(
                             "read failed: Resource temporarily unavailable"));
}

struct SysIFace_CloseFails {
  DEFAULT_READ;
  DEFAULT_WRITE;
  DEFAULT_POLL;

  static auto GetError() {
    return EIO;
  }

  static int Close(int fd) {
    (void)fd;
    return -1;
  }
};

TEST_CASE("TcpChannel Close fails", "[net]") {
  net::TcpChannel<SysIFace_CloseFails> c(1);
  REQUIRE(c.Alive());
  REQUIRE_THROWS_MATCHES(
      c.Close(),
      std::system_error,
      Catch::Matchers::Message("close failed: Input/output error"));
  REQUIRE_FALSE(c.Alive());
  c.Close();
}

struct SysIFace_PollFails {
  DEFAULT_READ;
  DEFAULT_WRITE;
  DEFAULT_CLOSE;

  static auto GetError() {
    return EFAULT;
  }

  static int Poll(struct pollfd* fds, nfds_t nfds, int timeout) {
    (void)fds;
    (void)nfds;
    (void)timeout;
    return -1;
  }
};

TEST_CASE("TcpChannel HasData fails", "[net]") {
  net::TcpChannel<SysIFace_PollFails> c(1);
  REQUIRE_THROWS_MATCHES(c.HasData(),
                         std::system_error,
                         Catch::Matchers::Message("poll failed: Bad address"));
}
