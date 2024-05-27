/* SCL --- Secure Computation Library
 * Copyright (C) 2024 Anders Dalskov
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

#ifndef SCL_SIMULATION_TRANSPORT_H
#define SCL_SIMULATION_TRANSPORT_H

#include <cstddef>
#include <deque>
#include <unordered_map>
#include <variant>

#include "scl/coro/task.h"
#include "scl/net/packet.h"
#include "scl/simulation/channel_id.h"
#include "scl/simulation/context.h"

namespace scl::sim::details {

/**
 * @brief Transport layer for a simulated network.
 *
 * Transport provides the functionality used when a simulated channel
 * sends or receives data. A Transport is shared between all parties
 * on the network, which allows it to e.g., only store one copy of a
 * packet even if it sent to multiple parties.
 */
class Transport final {
 private:
  // represents either an actual packet or an index to a packet. If
  // the variant is a packet, then it's because the packet was move'ed
  // to the receiver, whereas an index indicates that it was copied.
  using PktOrIdx = std::variant<net::Packet, std::size_t>;

  // An indirect packet transfer. The count indicates how many other
  // parties are waiting to receive the packet, and is incremented
  // when the packet is sent and decremented when the packet is
  // received. It is essentially a reference counter.
  struct PktAndCount {
    net::Packet packet;
    std::size_t count;
  };

 public:
  /**
   * @brief Send a packet on the transport.
   * @param cid the channel ID of the sending channel.
   * @param packet the packet to send.
   *
   * This function will attempt to directly move the packet to the
   * receiver.
   */
  void send(ChannelId cid, net::Packet&& packet);

  /**
   * @brief Send a packet on the transport.
   * @param cid the channel ID of the sending channel.
   * @param packet the packet.
   *
   * This function will attempt to only store one copy of the packet,
   * even if it is being sent to multiple parties. A copy of the
   * packet will happen when it is initially sent, and then once per
   * subsequent receive of the packet.
   */
  void send(ChannelId cid, const net::Packet& packet);

  /**
   * @brief Check if there's data for a channel on this transport.
   */
  bool hasData(ChannelId cid) const;

  /**
   * @brief Receive data on a channel.
   * @param cid the ID of the receiving channel.
   *
   * This function should only be called if there is data to be had on
   * the channel. Calling it in other cases is undefined behavior.
   */
  net::Packet recv(ChannelId cid);

  /**
   * @brief Performs some clean-up on the transport.
   *
   * This function will trim the internal lists of sent packets if no
   * more receivers are expected. Clean-up is performed as an explicit
   * separate step, because it might invalidate existing pointers (and
   * thus might not be "free" in terms of required computing).
   */
  void cleanUp(GlobalContext& ctx);

 private:
  // tracks p2p channels between parties
  std::unordered_map<ChannelId, std::deque<PktOrIdx>> m_channels;

  // tracks packets that are potentially sent to more than one
  // party. Each entry is a packet and a list of channels that the
  // packet is sent on.
  std::vector<PktAndCount> m_packets;
};

}  // namespace scl::sim::details

#endif  // SCL_SIMULATION_TRANSPORT_H
