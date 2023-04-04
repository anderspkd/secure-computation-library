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

#include "scl/net/network.h"

#include <thread>

#include "scl/net/channel.h"
#include "scl/net/mem_channel.h"
#include "scl/net/sys_iface.h"
#include "scl/net/tcp_utils.h"
#include "scl/net/threaded_sender.h"

scl::net::FakeNetwork scl::net::FakeNetwork::Create(unsigned id,
                                                    std::size_t n) {
  std::vector<std::shared_ptr<Channel>> channels;
  std::vector<std::shared_ptr<Channel>> remotes;
  channels.reserve(n);
  remotes.reserve(n);

  for (std::size_t i = 0; i < n; ++i) {
    if (i == id) {
      channels.emplace_back(MemoryBackedChannel::CreateLoopback());
      remotes.emplace_back(nullptr);
    } else {
      auto chls = MemoryBackedChannel::CreatePaired();
      channels.emplace_back(chls[0]);
      remotes.emplace_back(chls[1]);
    }
  }

  return FakeNetwork{id, Network{channels, id}, remotes};
}

std::vector<scl::net::Network> scl::net::CreateMemoryBackedNetwork(
    std::size_t n) {
  std::vector<std::vector<std::shared_ptr<Channel>>> channels(n);

  for (std::size_t i = 0; i < n; ++i) {
    channels[i] = std::vector<std::shared_ptr<Channel>>(n);
  }

  std::vector<Network> networks;
  networks.reserve(n);

  for (std::size_t i = 0; i < n; ++i) {
    channels[i][i] = MemoryBackedChannel::CreateLoopback();
    for (std::size_t j = i + 1; j < n; ++j) {
      auto chls = MemoryBackedChannel::CreatePaired();
      channels[i][j] = chls[0];
      channels[j][i] = chls[1];
    }
    networks.emplace_back(Network{channels[i], i});
  }

  return networks;
}
