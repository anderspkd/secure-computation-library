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

#ifndef SCL_NET_NETWORK_H
#define SCL_NET_NETWORK_H

#include <cstddef>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "scl/net/channel.h"
#include "scl/net/config.h"
#include "scl/net/mem_channel.h"
#include "scl/net/sys_iface.h"
#include "scl/net/tcp_channel.h"
#include "scl/net/tcp_utils.h"

namespace scl::net {

/**
 * @brief A Network.
 */
class Network {
 public:
  /**
   * @brief Create a network using a network config.
   * @param config the network configuration to use
   * @tparam ChannelT the channel type
   *
   * This creates a network where parties are connected using private
   * peer-to-peer channels.
   */
  template <typename ChannelT>
  static Network Create(const NetworkConfig& config);

  /**
   * @brief Create a new network.
   * @param channels a list of peer-to-peer channels
   * @param my_id the ID of the local party
   */
  Network(const std::vector<std::shared_ptr<Channel>>& channels,
          std::size_t my_id)
      : m_channels(channels), m_id(my_id){};

  Network() = default;

  /**
   * @brief Get a channel to a particular party.
   * @param id the id of the party
   */
  Channel* Party(unsigned id) {
    return m_channels[id].get();
  }

  /**
   * @brief Get the next party according to its ID.
   */
  Channel* Next() {
    const auto next_id = m_id == Size() - 1 ? 0 : m_id + 1;
    return m_channels[next_id].get();
  }

  /**
   * @brief Get the previous party according to its ID.
   */
  Channel* Previous() {
    const auto prev_id = m_id == 0 ? Size() - 1 : m_id - 1;
    return m_channels[prev_id].get();
  }

  /**
   * @brief Get the other party in the network.
   *
   * If the network has more than two parties then this method throws an
   * std::logic_error as the concept of "other" party is ambigious in that case.
   */
  Channel* Other() {
    if (Size() != 2) {
      throw std::logic_error("other party ambiguous for more than 2 parties");
    }
    return m_channels[1 - m_id].get();
  }

  /**
   * @brief The size of the network.
   */
  std::size_t Size() const {
    return m_channels.size();
  };

  /**
   * @brief The ID of the local party.
   */
  std::size_t MyId() const {
    return m_id;
  };

  /**
   * @brief Closes all channels in the network.
   */
  void Close() {
    for (auto& c : m_channels) {
      c->Close();
    }
  };

 private:
  std::vector<std::shared_ptr<Channel>> m_channels;
  std::size_t m_id;
};

/**
 * @brief A fake network. Useful for testing.
 */
struct FakeNetwork {
  /**
   * @brief Create a fake network of some size for a specific party.
   * @param id the ID of the party owning the fake network
   * @param n the size of the network
   * @return a FakeNetwork.
   */
  static FakeNetwork Create(unsigned id, std::size_t n);

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
 * @brief Create a fully connected network that resides in memory.
 * @param n the size of the network
 * @return a fully connected network.
 *
 * This function creates a list of networks of \p n parties where each pair of
 * parties are connected to eachother by a InMemoryChannel.
 */
std::vector<Network> CreateMemoryBackedNetwork(std::size_t n);

template <typename ChannelT>
Network Network::Create(const NetworkConfig& config) {
  std::vector<std::shared_ptr<Channel>> channels(config.NetworkSize());

  // connect to ourselves.
  channels[config.Id()] = MemoryBackedChannel::CreateLoopback();

  // This thread runs a server which accepts connections from all parties with
  // an ID strictly greater than ours.
  std::thread connector([&channels, &config]() {
    const auto id = config.Id();

    // the number of connections we should listen for.
    const auto m = config.NetworkSize() - id - 1;

    if (m > 0) {
      auto port = config.GetParty(id).port;
      auto server_socket = CreateServerSocket<>((int)port, (int)m);

      for (std::size_t i = id + 1; i < config.NetworkSize(); ++i) {
        auto conn = AcceptConnection(server_socket);
        std::shared_ptr<Channel> channel =
            std::make_shared<ChannelT>(conn.socket);

        auto p = channel->Recv().value();
        channels[p.Read<unsigned>()] = channel;
      }
      SysIFace::Close(server_socket);
    }
  });

  Packet p;
  for (std::size_t i = 0; i < config.Id(); ++i) {
    const auto party = config.GetParty(i);
    auto socket = ConnectAsClient<>(party.hostname, (int)party.port);
    std::shared_ptr<Channel> channel = std::make_shared<ChannelT>(socket);
    p << (unsigned)config.Id();
    channel->Send(p);
    channels[i] = channel;
    p.ResetWritePtr();
  }

  connector.join();
  return Network{channels, config.Id()};
}

}  // namespace scl::net

#endif  // SCL_NET_NETWORK_H
