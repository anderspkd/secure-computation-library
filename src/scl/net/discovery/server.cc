/**
 * @file server.cc
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

#include "scl/net/discovery/server.h"

#include <cstddef>
#include <memory>
#include <vector>

#include "scl/net/tcp_channel.h"
#include "scl/net/tcp_utils.h"

using Server = scl::DiscoveryServer;

scl::NetworkConfig Server::Run(const scl::Party& me) const {
  // one of the parties is us, which we do not connect to.
  int backlog = static_cast<int>(mNumberOfParties - 1);
  auto ssock = scl::details::CreateServerSocket(mPort, backlog);
  std::vector<std::shared_ptr<scl::Channel>> channels;
  std::vector<std::string> hostnames;

  for (std::size_t i = 0; i < mNumberOfParties; ++i) {
    if (i == static_cast<std::size_t>(me.id)) {
      channels.emplace_back(nullptr);
      hostnames.emplace_back(me.hostname);
    } else {
      auto ac = scl::details::AcceptConnection(ssock);
      auto hostname = scl::details::GetAddress(ac);
      auto channel = std::make_shared<scl::TcpChannel>(ac.socket);
      channels.emplace_back(channel);
      hostnames.emplace_back(hostname);
    }
  }

  Server::CollectIdsAndPorts discovery(hostnames);
  Server::Ctx ctx{me, scl::Network{channels}};
  return scl::Evaluate(discovery, ctx);
}

Server::SendNetworkConfig Server::CollectIdsAndPorts::Run(Server::Ctx& ctx) {
  auto my_id = static_cast<std::size_t>(ctx.me.id);
  std::vector<scl::Party> parties(mHostnames.size());
  parties[my_id] = ctx.me;

  for (std::size_t i = 0; i < mHostnames.size(); ++i) {
    if (my_id != i) {
      int id;
      ctx.network.Party(i)->Recv(id);
      if (static_cast<std::size_t>(id) >= parties.size()) {
        throw std::logic_error("received invalid party ID");
      }
      int port;
      ctx.network.Party(i)->Recv(port);
      parties[id] = scl::Party{id, mHostnames[id], port};
    }
  }

  scl::NetworkConfig cfg(ctx.me.id, parties);
  return Server::SendNetworkConfig(cfg);
}

namespace {

void SendHostname(scl::Channel* channel, const std::string& hostname) {
  std::size_t len = hostname.size();
  const unsigned char* ptr =
      reinterpret_cast<const unsigned char*>(hostname.c_str());

  channel->Send(len);
  channel->Send(ptr, len);
}

void SendConfig(scl::Channel* channel, const scl::NetworkConfig& config) {
  channel->Send(config.NetworkSize());
  for (std::size_t i = 0; i < config.NetworkSize(); ++i) {
    auto party = config.Parties()[i];
    channel->Send(party.id);
    channel->Send(party.port);
    SendHostname(channel, party.hostname);
  }
}

}  // namespace
scl::NetworkConfig Server::SendNetworkConfig::Finalize(Server::Ctx& ctx) {
  std::size_t network_size = mConfig.NetworkSize();
  for (std::size_t i = 0; i < network_size; ++i) {
    if (i == static_cast<std::size_t>(mConfig.Id())) {
      continue;
    }

    auto* channel = ctx.network.Party(i);
    SendConfig(channel, mConfig);
  }

  return mConfig;
}
