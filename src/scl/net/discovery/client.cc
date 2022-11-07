/**
 * @file client.cc
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

#include "scl/net/discovery/client.h"

#include <memory>

#include "scl/net/tcp_channel.h"
#include "scl/net/tcp_utils.h"

using Client = scl::DiscoveryClient;

scl::NetworkConfig Client::Run(int id, int port) const {
  auto socket = scl::details::ConnectAsClient(mHostname, mPort);
  std::shared_ptr<scl::Channel> server =
      std::make_shared<scl::TcpChannel>(socket);

  Client::SendIdAndPort discovery(id, port);
  return scl::Evaluate(discovery, server);
}

Client::ReceiveNetworkConfig Client::SendIdAndPort::Run(
    const std::shared_ptr<scl::Channel>& ctx) const {
  ctx->Send(mId);
  ctx->Send(mPort);
  return Client::ReceiveNetworkConfig{mId};
}

namespace {

std::string ReceiveHostname(const std::shared_ptr<scl::Channel>& ctx) {
  std::size_t len;
  ctx->Recv(len);
  auto buf = std::make_unique<char[]>(len);
  ctx->Recv(reinterpret_cast<unsigned char*>(buf.get()), len);
  return std::string(buf.get(), buf.get() + len);
}

}  // namespace

scl::NetworkConfig Client::ReceiveNetworkConfig::Finalize(
    const std::shared_ptr<scl::Channel>& ctx) const {
  std::size_t number_of_parties;
  ctx->Recv(number_of_parties);

  std::vector<scl::Party> parties(number_of_parties);
  for (std::size_t i = 0; i < number_of_parties; ++i) {
    scl::Party party;
    ctx->Recv(party.id);
    ctx->Recv(party.port);
    auto hostname = ReceiveHostname(ctx);
    party.hostname = hostname;
    parties[party.id] = party;
  }

  return scl::NetworkConfig{mId, parties};
}
