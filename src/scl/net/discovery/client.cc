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

#include "scl/net/discovery/client.h"

#include <any>
#include <memory>

#include "scl/net/config.h"
#include "scl/net/discovery/discovery.h"
#include "scl/net/network.h"
#include "scl/net/tcp_channel.h"
#include "scl/net/tcp_utils.h"

using namespace scl;

net::NetworkConfig net::Discovery::RunClient(const std::string& server_hostname,
                                             std::size_t server_port,
                                             std::size_t my_id,
                                             std::size_t my_port) {
  auto socket = ConnectAsClient(server_hostname, (int)server_port);
  std::shared_ptr<Channel> channel = std::make_shared<TcpChannel<>>(socket);
  Network network({channel, nullptr}, 1);

  std::unique_ptr<proto::Protocol> d =
      std::make_unique<DiscoveryClient::SendInfo>(my_id, my_port);
  NetworkConfig config;

  proto::Evaluate(std::move(d), network, [&config](std::any output) {
    config = std::any_cast<NetworkConfig>(output);
  });

  return config;
}

std::unique_ptr<proto::Protocol> net::DiscoveryClient::SendInfo::Run(
    proto::ProtocolEnvironment& env) {
  env.network.Party(0)->Send(mId);
  env.network.Party(0)->Send(mPort);

  return std::make_unique<DiscoveryClient::RecvConfig>(mId);
}

namespace {

std::string RecvHostname(net::Channel* server) {
  std::size_t sl;
  server->Recv(sl);
  auto buf = std::make_unique<char[]>(sl);
  server->Recv((unsigned char*)buf.get(), sl);
  return std::string(buf.get(), buf.get() + sl);
}

}  // namespace

std::unique_ptr<proto::Protocol> net::DiscoveryClient::RecvConfig::Run(
    proto::ProtocolEnvironment& env) {
  std::size_t n;
  env.network.Party(0)->Recv(n);

  std::vector<Party> parties(n);
  for (std::size_t i = 0; i < n; ++i) {
    Party party;
    env.network.Party(0)->Recv(party.id);
    env.network.Party(0)->Recv(party.port);
    party.hostname = RecvHostname(env.network.Party(0));
    parties[party.id] = party;
  }

  mConfig = NetworkConfig{mId, parties};

  return nullptr;
}
