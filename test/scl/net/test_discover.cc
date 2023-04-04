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

#include <any>
#include <catch2/catch.hpp>
#include <iostream>
#include <thread>

#include "scl/net/config.h"
#include "scl/net/discovery/client.h"
#include "scl/net/discovery/discovery.h"
#include "scl/net/discovery/server.h"
#include "scl/net/mem_channel.h"
#include "scl/net/network.h"
#include "scl/protocol/base.h"
#include "scl/protocol/env.h"
#include "scl/simulation/config.h"
#include "scl/simulation/simulator.h"

using namespace scl;

namespace {

bool VerifyParty(net::Party& party,
                 std::size_t id,
                 const std::string& hostname,
                 std::size_t port) {
  return party.id == id && party.hostname == hostname && party.port == port;
}

bool PartyEquals(net::Party& first, net::Party& second) {
  return first.id == second.id && first.hostname == second.hostname &&
         first.port == second.port;
}

}  // namespace

TEST_CASE("Discovery server RecvInfo", "[net]") {
  std::vector<std::string> hostnames = {"1.2.3.4", "4.4.4.4", "127.0.0.1"};
  net::Party me{1, "4.4.4.4", 1234};
  net::DiscoveryServer::RecvInfo prot(me, hostnames);
  auto fake = net::FakeNetwork::Create(me.id, hostnames.size());
  auto chls = fake.incoming;

  chls[0]->Send((std::size_t)0);
  chls[0]->Send((std::size_t)5555);

  chls[2]->Send((std::size_t)2);
  chls[2]->Send((std::size_t)2222);

  proto::ProtocolEnvironment env{fake.my_network, nullptr, nullptr};
  auto next = prot.Run(env);
  auto nptr = next->Run(env);
  REQUIRE(nptr == nullptr);

  auto cfg = std::any_cast<net::NetworkConfig>(next->Output());

  REQUIRE(cfg.Id() == me.id);
  REQUIRE(cfg.NetworkSize() == 3);

  REQUIRE(VerifyParty(cfg.Parties()[0], 0, "1.2.3.4", 5555));
  REQUIRE(VerifyParty(cfg.Parties()[1], 1, "4.4.4.4", 1234));
  REQUIRE(VerifyParty(cfg.Parties()[2], 2, "127.0.0.1", 2222));
}

TEST_CASE("Discovery server RecvInfo out-of-order", "[net]") {
  std::vector<std::string> hostnames = {"1.2.3.4", "4.4.4.4", "127.0.0.1"};
  net::Party me{1, "4.4.4.4", 1234};
  net::DiscoveryServer::RecvInfo prot(me, hostnames);
  auto fake = net::FakeNetwork::Create(me.id, hostnames.size());
  auto chls = fake.incoming;

  chls[0]->Send((std::size_t)2);
  chls[0]->Send((std::size_t)5555);

  chls[2]->Send((std::size_t)0);
  chls[2]->Send((std::size_t)2222);

  proto::ProtocolEnvironment env{fake.my_network, nullptr, nullptr};
  auto next = prot.Run(env);
  auto nptr = next->Run(env);
  REQUIRE(nptr == nullptr);

  auto cfg = std::any_cast<net::NetworkConfig>(next->Output());

  REQUIRE(cfg.Id() == me.id);
  REQUIRE(cfg.NetworkSize() == 3);

  REQUIRE(VerifyParty(cfg.Parties()[0], 0, "1.2.3.4", 2222));
  REQUIRE(VerifyParty(cfg.Parties()[1], 1, "4.4.4.4", 1234));
  REQUIRE(VerifyParty(cfg.Parties()[2], 2, "127.0.0.1", 5555));
}

TEST_CASE("Discovery server RecvInfo receive invalid ID", "[net]") {
  std::vector<std::string> hostnames = {"1.2.3.4", "4.4.4.4", "127.0.0.1"};
  net::Party me{1, "4.4.4.4", 1234};
  net::DiscoveryServer::RecvInfo prot(me, hostnames);
  auto fake = net::FakeNetwork::Create(me.id, hostnames.size());
  auto chls = fake.incoming;

  chls[0]->Send((std::size_t)42);
  chls[0]->Send((std::size_t)5555);

  proto::ProtocolEnvironment env{fake.my_network, nullptr, nullptr};
  REQUIRE_THROWS_MATCHES(prot.Run(env),
                         std::logic_error,
                         Catch::Matchers::Message("received invalid party ID"));
}

TEST_CASE("Discovery server SendConfig", "[net]") {
  auto fake = net::FakeNetwork::Create(1, 4);
  auto network = fake.my_network;
  auto channels = fake.incoming;

  auto cfg = net::NetworkConfig::Localhost(1, 4);
  auto me = cfg.Parties()[1];

  net::DiscoveryServer::SendConfig prot(cfg);

  proto::ProtocolEnvironment env{fake.my_network, nullptr, nullptr};

  REQUIRE(prot.Run(env) == nullptr);
  auto result = std::any_cast<net::NetworkConfig>(prot.Output());

  REQUIRE(result.Id() == cfg.Id());
  REQUIRE(result.NetworkSize() == cfg.NetworkSize());

  for (std::size_t i = 0; i < result.NetworkSize(); ++i) {
    REQUIRE(PartyEquals(result.Parties()[i], cfg.Parties()[i]));
  }
}

namespace {

void SendHostname(net::Channel* channel, const std::string& hostname) {
  std::size_t length = hostname.size();
  channel->Send(length);
  channel->Send(reinterpret_cast<const unsigned char*>(hostname.c_str()),
                length);
}

void SendConfig(net::Channel* channel, const net::NetworkConfig& config) {
  channel->Send(config.NetworkSize());
  for (const auto& party : config.Parties()) {
    channel->Send(party.id);
    channel->Send(party.port);
    SendHostname(channel, party.hostname);
  }
}

}  // namespace

TEST_CASE("Discovery client SendInfo", "[net]") {
  auto channels = net::MemoryBackedChannel::CreatePaired();
  std::shared_ptr<net::Channel> c0 = channels[0];
  std::shared_ptr<net::Channel> c1 = channels[1];
  net::DiscoveryClient::SendInfo prot(1, 5566);

  net::Network network({c0, nullptr}, 1);
  proto::ProtocolEnvironment env{network, nullptr, nullptr};
  prot.Run(env);

  std::size_t id;
  c1->Recv(id);
  REQUIRE(id == 1);

  std::size_t port;
  c1->Recv(port);
  REQUIRE(port == 5566);
}

TEST_CASE("Discovery client RecvConfig", "[net]") {
  auto channels = net::MemoryBackedChannel::CreatePaired();
  std::shared_ptr<net::Channel> c0 = channels[0];
  std::shared_ptr<net::Channel> c1 = channels[1];

  auto config = net::NetworkConfig::Localhost(0, 3, 4444);
  SendConfig(c1.get(), config);
  net::DiscoveryClient::RecvConfig prot(1);

  net::Network network({c0, nullptr}, 1);
  proto::ProtocolEnvironment env{network, nullptr, nullptr};
  REQUIRE(prot.Run(env) == nullptr);

  auto received = std::any_cast<net::NetworkConfig>(prot.Output());

  REQUIRE(received.Id() == 1);
  REQUIRE(received.NetworkSize() == config.NetworkSize());
  for (std::size_t i = 0; i < 3; ++i) {
    auto p0 = received.Parties()[i];
    auto p1 = config.Parties()[i];
    REQUIRE(PartyEquals(p0, p1));
  }
}

TEST_CASE("Discovery more than max parties", "[net]") {
  net::Party me{0, "", 0};
  REQUIRE_THROWS_MATCHES(
      net::Discovery::RunServer(MAX_DISCOVER_PARTIES + 1, me),
      std::invalid_argument,
      Catch::Matchers::Message("number of parties to discover exceeds max"));
}

TEST_CASE("Discovery", "[net]") {
  net::NetworkConfig c0;
  net::NetworkConfig c1;
  net::NetworkConfig c2;

  std::thread server([&c0]() {
    net::Party me{1, "127.0.0.1", 9999};
    c0 = net::Discovery::RunServer(3, me);
  });

  std::thread client0(
      [&c1]() { c1 = net::Discovery::RunClient("127.0.0.1", 9999, 0, 6666); });

  std::thread client1(
      [&c2]() { c2 = net::Discovery::RunClient("127.0.0.1", 9999, 2, 4444); });

  server.join();
  client0.join();
  client1.join();

  REQUIRE(c0.Id() == 1);
  REQUIRE(c1.Id() == 0);
  REQUIRE(c2.Id() == 2);

  REQUIRE(c0.NetworkSize() == 3);
  REQUIRE(c1.NetworkSize() == 3);
  REQUIRE(c2.NetworkSize() == 3);

  for (std::size_t i = 0; i < 3; ++i) {
    auto p0 = c0.Parties()[i];
    auto p1 = c1.Parties()[i];
    auto p2 = c2.Parties()[i];

    REQUIRE(PartyEquals(p0, p1));
    REQUIRE(PartyEquals(p1, p2));
  }
}

TEST_CASE("Discovery sim", "[net][.]") {
  const auto n = 5;

  auto creator = []() {
    std::vector<std::unique_ptr<proto::Protocol>> parties;
    std::vector<std::string> hostnames(n, "127.0.0.1");
    net::Party server{0, "127.0.0.1", 1234};
    parties.emplace_back(
        std::make_unique<net::DiscoveryServer::RecvInfo>(server, hostnames));
    for (std::size_t i = 1; i < n; ++i) {
      parties.emplace_back(
          std::make_unique<net::DiscoveryClient::SendInfo>(i, 1234));
    }

    return parties;
  };

  auto r = sim::Simulate(creator, sim::DefaultConfigCreator(), 10);

  for (const auto& result : r) {
    std::cout << result.ExecutionTime() << "\n";
    std::cout << result.TransferAmounts().recv << "\n";
    std::cout << result.TransferAmounts().sent << "\n";
    std::cout << "--\n";
  }
}
