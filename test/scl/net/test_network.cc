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
#include <cstdint>
#include <thread>

#include "scl/net/config.h"
#include "scl/net/network.h"
#include "scl/net/tcp_channel.h"

using namespace scl;

namespace {

net::Packet GetPacketWithData() {
  net::Packet p;
  p << (int)123;
  p << (float)4.5;
  return p;
}

void CheckPacket(std::optional<net::Packet>& p) {
  if (p.has_value()) {
    REQUIRE(p.value().Read<int>() == 123);
    REQUIRE(p.value().Read<float>() == 4.5);
  } else {
    FAIL("packet did not have data");
  }
}

}  // namespace

TEST_CASE("Network fake", "[net]") {
  auto fake = net::FakeNetwork::Create(0, 3);

  auto network = fake.my_network;
  auto remotes = fake.incoming;

  REQUIRE(network.Size() == 3);
  REQUIRE(remotes[0] == nullptr);

  auto p = GetPacketWithData();

  remotes[1]->Send(p);

  auto rp1 = network.Party(1)->Recv();
  CheckPacket(rp1);

  p.ResetReadPtr();
  network.Party(0)->Send(p);
  auto rp0 = network.Party(0)->Recv();
  CheckPacket(rp0);
}

TEST_CASE("Network fully connected", "[net]") {
  auto networks = net::CreateMemoryBackedNetwork(3);
  REQUIRE(networks.size() == 3);

  auto network0 = networks[0];
  auto network1 = networks[1];

  auto p = GetPacketWithData();

  // p0 -> p1
  network0.Party(1)->Send(p);

  // p1 <- p0
  auto p10 = network1.Party(0)->Recv();
  CheckPacket(p10);

  auto network2 = networks[2];
  // p2 -> p0
  network2.Party(0)->Send(p);

  // p0 <- p2
  auto p02 = network0.Party(2)->Recv();
  CheckPacket(p02);
}

TEST_CASE("Network TCP", "[net]") {
  net::Network network0;
  net::Network network1;
  net::Network network2;

  std::thread t0([&]() {
    network0 = net::Network::Create<net::TcpChannel<>>(
        net::NetworkConfig::Localhost(0, 3));
  });
  std::thread t1([&]() {
    network1 = net::Network::Create<net::TcpChannel<>>(
        net::NetworkConfig::Localhost(1, 3));
  });
  std::thread t2([&]() {
    network2 = net::Network::Create<net::TcpChannel<>>(
        net::NetworkConfig::Localhost(2, 3));
  });

  t0.join();
  t1.join();
  t2.join();

  for (std::size_t i = 0; i < 3; ++i) {
    // Alive doesn't exist on InMemoryChannel
    if (i != 0) {
      REQUIRE(((net::TcpChannel<>*)network0.Party(i))->Alive());
    }
    if (i != 1) {
      REQUIRE(((net::TcpChannel<>*)network1.Party(i))->Alive());
    }
    if (i != 2) {
      REQUIRE(((net::TcpChannel<>*)network2.Party(i))->Alive());
    }
  }

  auto p = GetPacketWithData();

  network0.Party(2)->Send(p);

  auto p20 = network2.Party(0)->Recv();
  CheckPacket(p20);
}

struct ChannelMock final : net::Channel {
  void Close() override {
    close_called++;
  }

  void Send(const unsigned char* src, std::size_t n) override {
    (void)src;
    (void)n;
    send_called++;
  }

  std::size_t Recv(unsigned char* dst, std::size_t n) override {
    (void)dst;
    (void)n;
    return 0;
  }

  bool HasData() override {
    return false;
  }

  std::size_t close_called = 0;
  std::size_t send_called = 0;
};

TEST_CASE("Network party getters") {
  const auto chl0 = std::make_shared<ChannelMock>();
  const auto chl1 = std::make_shared<ChannelMock>();
  const auto chl2 = std::make_shared<ChannelMock>();

  net::Network nw({chl0, chl1, chl2}, 1);

  auto p = GetPacketWithData();
  REQUIRE(chl2->send_called == 0);
  nw.Next()->Send(p);
  REQUIRE(chl2->send_called == 2);

  REQUIRE(chl0->send_called == 0);
  nw.Previous()->Send(p);
  REQUIRE(chl0->send_called == 2);

  REQUIRE_THROWS_MATCHES(nw.Other(),
                         std::logic_error,
                         Catch::Matchers::Message(
                             "other party ambiguous for more than 2 parties"));

  net::Network two_parties({chl0, chl1}, 1);

  two_parties.Other()->Send(p);
  REQUIRE(chl0->send_called == 4);
}

TEST_CASE("Network close") {
  const auto chl0 = std::make_shared<ChannelMock>();
  const auto chl1 = std::make_shared<ChannelMock>();
  const auto chl2 = std::make_shared<ChannelMock>();

  net::Network nw({chl0, chl1, chl2}, 1);

  REQUIRE(chl0->close_called == 0);
  REQUIRE(chl1->close_called == 0);
  REQUIRE(chl2->close_called == 0);

  nw.Close();

  REQUIRE(chl0->close_called == 1);
  REQUIRE(chl1->close_called == 1);
  REQUIRE(chl2->close_called == 1);
}
