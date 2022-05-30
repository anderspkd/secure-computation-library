/**
 * @file network.h
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

#ifndef _SCL_NET_NETWORK_H
#define _SCL_NET_NETWORK_H

#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "scl/net/channel.h"
#include "scl/net/config.h"
#include "scl/net/mem_channel.h"
#include "scl/net/tcp_channel.h"

namespace scl {

/**
 * @brief A collection of channels.
 */
class Network {
 public:
  /**
   * @brief Create a network using a network config.
   *
   * This creates a network where parties are connected using private
   * peer-to-peer channels over TPC.
   *
   * @param config the network configuration to use
   */
  static Network Create(const NetworkConfig& config);

  /**
   * @brief Create a network using a network config.
   *
   * This creates a network where parties are connected using private
   * peer-to-peer channels over TPC. The channels are wrapped in the
   * scl::ThreadedSender decorator.
   *
   * @param config the config
   */
  static Network CreateThreadedSenders(const NetworkConfig& config);

  /**
   * @brief Create a mock network.
   *
   * This creates a network and a list of channels where the network will
   * receive data sent on the channels. This allows precisely controling the
   * network input to a particular party.
   *
   * @param id the ID of the party owning the network
   * @param n the number of channels (or fake parties) to create
   */
  static std::pair<Network, std::vector<std::shared_ptr<Channel>>> CreateMock(
      unsigned id, std::size_t n);

  /**
   * @brief Creates a list of n in-memory networks.
   *
   * This creates a list of \p n networks that are connected in memory. The
   * network of party i is the i'th network returned.
   *
   * @param n the number of networks to create
   */
  static std::vector<Network> CreateFullInMemory(std::size_t n);

  Network() = default;

  /**
   * @brief Create a new network.
   * @param channels a list of peer-to-peer channels
   */
  Network(const std::vector<std::shared_ptr<Channel>>& channels)
      : mChannels(channels){};

  /**
   * @brief Get a channel to a particular party.
   * @param id the id of the party
   */
  Channel* Party(unsigned id) { return mChannels[id].get(); }

  /**
   * @brief The size of the network.
   */
  std::size_t Size() const { return mChannels.size(); };

  /**
   * @brief Closes all channels in the network.
   */
  void Close() {
    for (auto c : mChannels) c->Close();
  };

 private:
  std::vector<std::shared_ptr<Channel>> mChannels;
};

}  // namespace scl

#endif  // _SCL_NET_NETWORK_H
