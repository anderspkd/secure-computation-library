/**
 * @file test_threaded_sender.cc
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

#include <catch2/catch.hpp>
#include <iostream>
#include <memory>
#include <thread>

#include "scl/net/tcp_utils.h"
#include "scl/net/threaded_sender.h"
#include "scl/prg.h"
#include "util.h"

TEST_CASE("ThreadedSender", "[network]") {
  SECTION("Connect and send") {
    auto port = scl_tests::GetPort();

    std::shared_ptr<scl::Channel> client, server;

    std::thread clt([&]() {
      int socket = scl::details::ConnectAsClient("0.0.0.0", port);
      client = std::make_shared<scl::ThreadedSenderChannel>(socket);
    });

    std::thread srv([&]() {
      int ssock = scl::details::CreateServerSocket(port, 1);
      auto ac = scl::details::AcceptConnection(ssock);
      server = std::make_shared<scl::ThreadedSenderChannel>(ac.socket);
      scl::details::CloseSocket(ssock);
    });

    clt.join();
    srv.join();

    scl::PRG prg;
    unsigned char send[200] = {0};
    unsigned char recv[200] = {0};
    prg.Next(send, 200);

    client->Send(send, 100);
    client->Send(send + 100, 100);

    server->Recv(recv, 20);
    server->Recv(recv + 20, 180);

    client->Close();
    server->Close();

    REQUIRE(scl_tests::BufferEquals(send, recv, 200));
  }
}
