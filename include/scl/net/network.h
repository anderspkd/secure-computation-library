/* SCL --- Secure Computation Library
 * Copyright (C) 2025 Anders Dalskov
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

#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "scl/coro/task.h"
#include "scl/net/channel.h"

namespace scl {

/**
 * @brief A Network.
 *
 * <p>A Network is effectively a list of Channel's with a bunch of helper
 * functions and is the main interface that an MPC protocol will use to
 * communicate with other parties.
 *
 * <p>Below is shown how a typical Beaver multiplication might be carried out
 * using a Network object to communicate:
 *
 * @code
 * Network nw = ... // initialize in some way
 *
 * for (int i = 0; i < nw.size(); i++) {
 *   Packet pkt = getDataToSend();
 *   co_await nw.party(i)->send(pkt);
 * }
 *
 * for (int i = 0; i < nw.size(); i++) {
 *   auto recvd = co_await nw.party(i)->recv();
 *   processReceivedData(recvd);
 * }
 * @endcode
 */
class Network {
 public:
  /**
   * @brief Create a new network.
   * @param channels the list of channels in the network.
   * @param id the ID of the local party
   */
  Network(const std::vector<std::shared_ptr<Channel>>& channels, std::size_t id)
      : m_channels(channels), m_id(id) {};

  Network() = default;

  /**
   * @brief Get a communication channel to some party.
   * @param id the id of the party.
   * @return the channel to party \p id.
   */
  Channel* party(unsigned id) {
    return m_channels[id].get();
  }

  /**
   * @brief Get the next party according to its ID.
   * @return channel to the party with ID <code>(myId() + 1) % size()</code>.
   */
  Channel* next() {
    const auto next_id = m_id == size() - 1 ? 0 : m_id + 1;
    return m_channels[next_id].get();
  }

  /**
   * @brief Get the previous party according to its ID.
   * @return channel to the party with ID <code>(myId() - 1) % size()</code>.
   */
  Channel* previous() {
    const auto prev_id = m_id == 0 ? size() - 1 : m_id - 1;
    return m_channels[prev_id].get();
  }

  /**
   * @brief Get the other party in the network.
   * @return channel to the party with ID <code>1 - myId()</code>.
   * @throws std::logic error if the network contains more than two parties.
   *
   * This function is only meaningful for a two-party network.
   */
  Channel* other() {
    if (size() != 2) {
      throw std::logic_error("other party ambiguous for more than 2 parties");
    }
    return m_channels[1 - m_id].get();
  }

  /**
   * @brief Get the channel to the local party.
   * @return the channel to the party with ID equal to myId().
   */
  Channel* me() {
    return party(myId());
  }

  /**
   * @brief Send a packet to all parties on this network.
   * @param packet the packet.
   *
   * This function is equivalent to
   * @code
   * for (int i = 0; i < size(); i++) {
   *   co_await party(i)->send(packet);
   * }
   * @endcode
   */
  Task<void> send(const Packet& packet) {
    for (std::size_t i = 0; i < size(); i++) {
      co_await party(i)->send(packet);
    }
  }

  /**
   * @brief The number of parties in this network.
   */
  std::size_t size() const {
    return m_channels.size();
  };

  /**
   * @brief The ID of the local party.
   */
  std::size_t myId() const {
    return m_id;
  };

  /**
   * @brief Closes all channels in the network.
   */
  void close() {
    for (auto& c : m_channels) {
      c->close();
    }
  };

 private:
  std::vector<std::shared_ptr<Channel>> m_channels;
  std::size_t m_id;
};

}  // namespace scl
