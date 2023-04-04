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
#include <iostream>
#include <memory>
#include <thread>

#include "scl/net/sys_iface.h"
#include "scl/net/tcp_utils.h"
#include "scl/net/threaded_sender.h"
#include "scl/util/prg.h"
#include "util.h"

using namespace scl;

TEST_CASE("ThreadedSender send/recv", "[network]") {
  auto port = test::GetPort();

  std::shared_ptr<net::Channel> client;
  std::shared_ptr<net::Channel> server;

  std::thread clt([&]() {
    int socket = net::ConnectAsClient("0.0.0.0", port);
    client = std::make_shared<net::ThreadedSenderChannel>(socket);
  });

  std::thread srv([&]() {
    int ssock = net::CreateServerSocket(port, 1);
    auto ac = net::AcceptConnection(ssock);
    server = std::make_shared<net::ThreadedSenderChannel>(ac.socket);
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

  // because the sender returns immediately, there might not be data
  // available, so we will try a couple of times before failing.
  {
    using namespace std::chrono_literals;
    auto c = 0;
    while (c < 10 && !server->HasData()) {
      std::this_thread::sleep_for(100ms);
      c++;
    }
  }
  REQUIRE(server->HasData());

  server->Recv(recv, 20);
  server->Recv(recv + 20, 180);

  client->Close();
  server->Close();

  REQUIRE(test::BufferEquals(send, recv, 200));
}
