/**
 * @file network.cc
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

#include "scl/net/network.h"

#include <thread>

#include "scl/net/tcp_utils.h"
#include "scl/net/threaded_sender.h"

static inline std::shared_ptr<scl::Channel> Self() {
  return scl::InMemoryChannel::CreateSelfConnecting();
}

static inline void AcceptConnections(
    std::vector<std::shared_ptr<scl::Channel>>& channels,
    const scl::NetworkConfig config) {
  // Act as server for all clients with an ID strictly greater than ours.
  auto my_id = config.Id();
  auto n = config.NetworkSize() - my_id - 1;
  if (n) {
    auto port = config.GetParty(my_id).port;
    int ssock = scl::details::CreateServerSocket(port, n);
    for (std::size_t i = config.Id() + 1; i < config.NetworkSize(); ++i) {
      auto ac = scl::details::AcceptConnection(ssock);
      std::shared_ptr<scl::Channel> channel =
          std::make_shared<scl::TcpChannel>(ac.socket);
      unsigned id;
      channel->Recv(id);
      channels[id] = channel;
    }
    scl::details::CloseSocket(ssock);
  }
}

scl::Network scl::Network::Create(const scl::NetworkConfig& config) {
  std::vector<std::shared_ptr<scl::Channel>> channels(config.NetworkSize());

  channels[config.Id()] = Self();

  std::thread server(AcceptConnections, std::ref(channels), config);

  for (std::size_t i = 0; i < config.Id(); ++i) {
    const auto party = config.GetParty(i);
    auto socket = scl::details::ConnectAsClient(party.hostname, party.port);
    std::shared_ptr<scl::Channel> channel =
        std::make_shared<scl::TcpChannel>(socket);
    channel->Send((unsigned)config.Id());
    channels[i] = channel;
  }

  server.join();

  return Network{channels};
}

scl::Network scl::Network::CreateThreadedSenders(const NetworkConfig& config) {
  // TODO: The code here is identical to the above method with the exception of
  // which instantiation is used. This will be refactored at some point.

  std::vector<std::shared_ptr<scl::Channel>> channels(config.NetworkSize());

  channels[config.Id()] = Self();

  std::thread server(AcceptConnections, std::ref(channels), config);

  for (std::size_t i = 0; i < config.Id(); ++i) {
    const auto party = config.GetParty(i);
    auto socket = scl::details::ConnectAsClient(party.hostname, party.port);
    std::shared_ptr<scl::Channel> channel =
        std::make_shared<scl::ThreadedSenderChannel>(socket);
    channel->Send((unsigned)config.Id());
    channels[i] = channel;
  }

  server.join();

  return Network{channels};
}

using BuildMockNetwork_ReturnT =
    std::pair<scl::Network, std::vector<std::shared_ptr<scl::Channel>>>;

BuildMockNetwork_ReturnT scl::Network::CreateMock(unsigned id, std::size_t n) {
  std::vector<std::shared_ptr<scl::Channel>> channels;
  std::vector<std::shared_ptr<scl::Channel>> remotes;
  channels.reserve(n);
  remotes.reserve(n);

  for (std::size_t i = 0; i < n; ++i) {
    if (i == id) {
      channels.emplace_back(Self());
      remotes.emplace_back(nullptr);
    } else {
      auto chls = scl::InMemoryChannel::CreatePaired();
      channels.emplace_back(chls[0]);
      remotes.emplace_back(chls[1]);
    }
  }

  return BuildMockNetwork_ReturnT{Network{channels}, remotes};
}

std::vector<scl::Network> scl::Network::CreateFullInMemory(std::size_t n) {
  std::vector<std::vector<std::shared_ptr<scl::Channel>>> channels(n);

  for (std::size_t i = 0; i < n; ++i) {
    channels[i] = std::vector<std::shared_ptr<scl::Channel>>(n);
  }

  std::vector<scl::Network> networks;
  networks.reserve(n);

  for (std::size_t i = 0; i < n; ++i) {
    channels[i][i] = Self();
    for (std::size_t j = i + 1; j < n; ++j) {
      auto chls = scl::InMemoryChannel::CreatePaired();
      channels[i][j] = chls[0];
      channels[j][i] = chls[1];
    }
    networks.emplace_back(Network{channels[i]});
  }

  return networks;
}
