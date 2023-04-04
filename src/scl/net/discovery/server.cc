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

#include "scl/net/discovery/server.h"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <vector>

#include "scl/net/config.h"
#include "scl/net/discovery/discovery.h"
#include "scl/net/tcp_channel.h"
#include "scl/net/tcp_utils.h"
#include "scl/protocol/base.h"

using namespace scl;

net::NetworkConfig net::Discovery::RunServer(std::size_t number_of_parties,
                                             const Party& me) {
  if (number_of_parties > MAX_DISCOVER_PARTIES) {
    throw std::invalid_argument("number of parties to discover exceeds max");
  }

  int backlog = (int)(number_of_parties - 1);
  auto server_sock = CreateServerSocket<>((int)me.port, backlog);
  std::vector<std::shared_ptr<Channel>> channels;
  std::vector<std::string> hostnames;

  for (std::size_t i = 0; i < number_of_parties; ++i) {
    if (i == me.id) {
      channels.emplace_back(nullptr);
      hostnames.emplace_back(me.hostname);
    } else {
      auto acc = AcceptConnection<>(server_sock);
      channels.emplace_back(std::make_shared<TcpChannel<>>(acc.socket));
      hostnames.emplace_back(acc.hostname);
    }
  }

  Network network(channels, me.id);
  auto p = std::make_unique<DiscoveryServer::RecvInfo>(me, hostnames);
  NetworkConfig config;

  proto::Evaluate(std::move(p), network, [&config](std::any output) {
    config = std::any_cast<NetworkConfig>(output);
  });

  return config;
}

std::unique_ptr<proto::Protocol> net::DiscoveryServer::RecvInfo::Run(
    proto::ProtocolEnvironment& env) {
  const auto me = mMe.id;
  std::vector<Party> parties(mHostnames.size());
  parties[me] = mMe;

  for (std::size_t i = 0; i < mHostnames.size(); ++i) {
    if (i != me) {
      std::size_t id;
      env.network.Party(i)->Recv(id);

      if (id >= parties.size()) {
        throw std::logic_error("received invalid party ID");
      }

      std::size_t port;
      env.network.Party(i)->Recv(port);
      parties[id] = Party{id, mHostnames[id], port};
    }
  }

  net::NetworkConfig config{mMe.id, parties};
  return std::make_unique<net::DiscoveryServer::SendConfig>(config);
}

namespace {

void SendHostname(scl::net::Channel* channel, const std::string& hostname) {
  std::size_t len = hostname.size();
  const unsigned char* ptr =
      reinterpret_cast<const unsigned char*>(hostname.c_str());

  channel->Send(len);
  channel->Send(ptr, len);
}

void SendNetworkConfig(scl::net::Channel* channel,
                       const scl::net::NetworkConfig& config) {
  channel->Send(config.NetworkSize());
  for (std::size_t i = 0; i < config.NetworkSize(); ++i) {
    auto party = config.Parties()[i];
    channel->Send(party.id);
    channel->Send(party.port);
    SendHostname(channel, party.hostname);
  }
}

}  // namespace

std::unique_ptr<proto::Protocol> net::DiscoveryServer::SendConfig::Run(
    proto::ProtocolEnvironment& env) {
  std::size_t sz = mConfig.NetworkSize();
  for (std::size_t i = 0; i < sz; ++i) {
    if (i != mConfig.Id()) {
      SendNetworkConfig(env.network.Party(i), mConfig);
    }
  }

  return nullptr;
}
