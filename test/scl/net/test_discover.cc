/**
 * @file test_discover.cc
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
#include <thread>

#include "scl/net/config.h"
#include "scl/net/discovery/client.h"
#include "scl/net/discovery/server.h"
#include "scl/net/mem_channel.h"

static inline bool VerifyParty(scl::Party& party, unsigned id,
                               std::string hostname, int port) {
  return party.id == id && party.hostname == hostname && party.port == port;
}

static inline bool PartyEquals(scl::Party& first, scl::Party& second) {
  return first.id == second.id && first.hostname == second.hostname &&
         first.port == second.port;
}

TEST_CASE("Discovery Server", "[network]") {
  SECTION("CollectIdsAndPorts") {
    std::vector<std::string> hostnames = {"1.2.3.4", "4.4.4.4", "127.0.0.1"};
    scl::Party me{1, "4.4.4.4", 1234};
    scl::DiscoveryServer::CollectIdsAndPorts prot(hostnames);
    auto mock = scl::Network::CreateMock(me.id, hostnames.size());
    auto chls = std::get<1>(mock);

    chls[0]->Send((unsigned)0);
    chls[0]->Send((int)5555);

    chls[2]->Send((unsigned)2);
    chls[2]->Send((int)2222);

    scl::DiscoveryServer::Ctx ctx{me, std::get<0>(mock)};
    auto next = prot.Run(ctx);
    auto cfg = next.Finalize(ctx);

    REQUIRE(cfg.Id() == me.id);
    REQUIRE(cfg.NetworkSize() == 3);

    REQUIRE(VerifyParty(cfg.Parties()[0], 0, "1.2.3.4", 5555));
    REQUIRE(VerifyParty(cfg.Parties()[1], 1, "4.4.4.4", 1234));
    REQUIRE(VerifyParty(cfg.Parties()[2], 2, "127.0.0.1", 2222));
  }

  SECTION("CollectIdsAndPorts out-of-order") {
    std::vector<std::string> hostnames = {"1.2.3.4", "4.4.4.4", "127.0.0.1"};
    scl::Party me{1, "4.4.4.4", 1234};
    scl::DiscoveryServer::CollectIdsAndPorts prot(hostnames);
    auto mock = scl::Network::CreateMock(me.id, hostnames.size());
    auto chls = std::get<1>(mock);

    chls[0]->Send((unsigned)2);
    chls[0]->Send((int)5555);

    chls[2]->Send((unsigned)0);
    chls[2]->Send((int)2222);

    scl::DiscoveryServer::Ctx ctx{me, std::get<0>(mock)};
    auto next = prot.Run(ctx);
    auto cfg = next.Finalize(ctx);

    REQUIRE(cfg.Id() == me.id);
    REQUIRE(cfg.NetworkSize() == 3);

    REQUIRE(VerifyParty(cfg.Parties()[0], 0, "1.2.3.4", 2222));
    REQUIRE(VerifyParty(cfg.Parties()[1], 1, "4.4.4.4", 1234));
    REQUIRE(VerifyParty(cfg.Parties()[2], 2, "127.0.0.1", 5555));
  }

  SECTION("CollectIdsAndPorts invalid ID") {
    std::vector<std::string> hostnames = {"1.2.3.4", "4.4.4.4", "127.0.0.1"};
    scl::Party me{1, "4.4.4.4", 1234};
    scl::DiscoveryServer::CollectIdsAndPorts prot(hostnames);
    auto mock = scl::Network::CreateMock(me.id, hostnames.size());
    auto chls = std::get<1>(mock);

    chls[0]->Send((unsigned)42);
    chls[0]->Send((int)5555);

    scl::DiscoveryServer::Ctx ctx{me, std::get<0>(mock)};
    REQUIRE_THROWS_MATCHES(
        prot.Run(ctx), std::logic_error,
        Catch::Matchers::Message("received invalid party ID"));
  }

  SECTION("SendNetworkConfig") {
    auto mock = scl::Network::CreateMock(1, 4);
    auto network = std::get<0>(mock);
    auto channels = std::get<1>(mock);

    auto cfg = scl::NetworkConfig::Localhost(1, 4);
    auto me = cfg.Parties()[1];
    scl::DiscoveryServer::Ctx ctx{me, network};

    scl::DiscoveryServer::SendNetworkConfig prot(cfg);

    auto result = prot.Finalize(ctx);

    REQUIRE(result.Id() == cfg.Id());
    REQUIRE(result.NetworkSize() == cfg.NetworkSize());

    for (std::size_t i = 0; i < result.NetworkSize(); ++i) {
      REQUIRE(PartyEquals(result.Parties()[i], cfg.Parties()[i]));
    }
  }
}

static inline void SendHostname(scl::Channel* channel, std::string hostname) {
  std::size_t length = hostname.size();
  channel->Send(length);
  channel->Send(reinterpret_cast<const unsigned char*>(hostname.c_str()),
                length);
}

static inline void SendConfig(scl::Channel* channel,
                              const scl::NetworkConfig& config) {
  channel->Send(config.NetworkSize());
  for (const auto& party : config.Parties()) {
    channel->Send(party.id);
    channel->Send(party.port);
    SendHostname(channel, party.hostname);
  }
}

TEST_CASE("Discovery Client", "[network]") {
  SECTION("SendIdAndPort") {
    auto channels = scl::InMemoryChannel::CreatePaired();
    std::shared_ptr<scl::Channel> c0 = channels[0];
    std::shared_ptr<scl::Channel> c1 = channels[1];
    scl::DiscoveryClient::SendIdAndPort prot(1, 5566);
    prot.Run(c0);

    unsigned id;
    c1->Recv(id);
    REQUIRE(id == 1);

    int port;
    c1->Recv(port);
    REQUIRE(port == 5566);
  }

  SECTION("RecveiveNetworkConfig") {
    auto channels = scl::InMemoryChannel::CreatePaired();
    std::shared_ptr<scl::Channel> c0 = channels[0];
    std::shared_ptr<scl::Channel> c1 = channels[1];

    auto config = scl::NetworkConfig::Localhost(0, 3, 4444);
    SendConfig(c1.get(), config);
    scl::DiscoveryClient::ReceiveNetworkConfig prot(1);

    auto received = prot.Finalize(c0);

    REQUIRE(received.Id() == 1);
    REQUIRE(received.NetworkSize() == config.NetworkSize());
    for (std::size_t i = 0; i < 3; ++i) {
      auto p0 = received.Parties()[i];
      auto p1 = config.Parties()[i];
      REQUIRE(PartyEquals(p0, p1));
    }
  }
}

TEST_CASE("Discovery", "[network]") {
  using Server = scl::DiscoveryServer;
  using Client = scl::DiscoveryClient;

  SECTION("Too many parties") {
    REQUIRE_THROWS_MATCHES(
        Server(9999, 256), std::invalid_argument,
        Catch::Matchers::Message("number_of_parties exceeds max"));

    REQUIRE_THROWS_MATCHES(
        Server(256), std::invalid_argument,
        Catch::Matchers::Message("number_of_parties exceeds max"));
  }

  scl::NetworkConfig c0, c1, c2;

  std::thread server([&c0]() {
    Server srv(9999, 3);
    scl::Party p{1, "127.0.0.1", 3333};
    c0 = srv.Run(p);
  });

  std::thread client0([&c1]() {
    Client clt("127.0.0.1", 9999);
    c1 = clt.Run(0, 6666);
  });

  std::thread client1([&c2]() {
    Client clt("127.0.0.1", 9999);
    c2 = clt.Run(2, 4444);
  });

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
