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

#ifndef SCL_NET_NETWORK_H
#define SCL_NET_NETWORK_H

#include <iostream>
#include <memory>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "scl/net/channel.h"
#include "scl/net/config.h"
#include "scl/net/tcp_utils.h"

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
   * peer-to-peer channels.
   *
   * @param config the network configuration to use
   * @tparam ChannelT the channel type
   */
  template <typename ChannelT>
  static Network Create(const NetworkConfig& config);

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

/**
 * @brief A fake network. Useful for testing.
 */
struct FakeNetwork {
  /**
   * @brief The ID of the party owning this fake network.
   */
  unsigned id;

  /**
   * @brief The network object held by the local party.
   */
  Network my_network;

  /**
   * @brief Channels that send data to the local party.
   *
   * The channel on index <code>i != id</code> of this list can be used to send
   * data to the local party. The channel on index <code>id</code> is a
   * <code>nullptr</code>.
   */
  std::vector<std::shared_ptr<Channel>> incoming;
};

/**
 * @brief Create a fake network of some size for a specific party.
 * @param id the ID of the party owning the fake network
 * @param n the size of the network
 * @return a FakeNetwork.
 */
FakeNetwork CreateFakeNetwork(unsigned id, std::size_t n);

/**
 * @brief Create a fully connected network that resides in memory.
 *
 * This function creates a fully connected network consisting of
 * scl::InMemoryChannel's. Data sent to party <code>j</code> on network
 * <code>i</code> can be received by receiving from <code>i</code> on network
 * <code>j</code>.
 *
 * @param n the size of the network
 * @return a fully connected network.
 */
std::vector<Network> CreateFullyConnectedInMemory(std::size_t n);

namespace details {

/**
 * @brief Create a channel that connects to itself.
 *
 * This simply creates a channel where calls to scl::Channel::Send can be read
 * by calling scl::Channel::Recv.
 */
std::shared_ptr<Channel> CreateChannelConnectingToSelf();

// Internal helper function for Network::Create. This is used by a server thread
// to construct channel objects for all parties where the ID is strictly greater
// than our own.
void SCL_AcceptConnections(std::vector<std::shared_ptr<Channel>>& channels,
                           const NetworkConfig& config);
}  // namespace details

template <typename ChannelT>
Network Network::Create(const scl::NetworkConfig& config) {
  std::vector<std::shared_ptr<scl::Channel>> channels(config.NetworkSize());

  channels[config.Id()] = scl::details::CreateChannelConnectingToSelf();

  std::thread server(scl::details::SCL_AcceptConnections, std::ref(channels),
                     config);

  for (std::size_t i = 0; i < config.Id(); ++i) {
    const auto party = config.GetParty(i);
    auto socket = scl::details::ConnectAsClient(party.hostname, party.port);
    std::shared_ptr<scl::Channel> channel = std::make_shared<ChannelT>(socket);
    channel->Send((unsigned)config.Id());
    channels[i] = channel;
  }

  server.join();

  return Network{channels};
}

}  // namespace scl

#endif  // SCL_NET_NETWORK_H
