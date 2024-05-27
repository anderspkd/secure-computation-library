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

#include "scl/simulation/transport.h"

using namespace scl;

void sim::details::Transport::send(sim::ChannelId cid, net::Packet&& packet) {
  m_channels[cid.flip()].push_back(std::move(packet));
}

void sim::details::Transport::send(sim::ChannelId cid,
                                   const net::Packet& packet) {
  std::size_t idx;
  for (idx = 0; idx < m_packets.size(); idx++) {
    if (m_packets[idx].packet == packet) {
      m_packets[idx].count++;
      m_channels[cid.flip()].push_back(idx);
      return;
    }
  }

  // no packet found
  m_packets.emplace_back(PktAndCount{packet, 1});
  m_channels[cid.flip()].push_back(idx);
}

bool sim::details::Transport::hasData(ChannelId cid) const {
  if (m_channels.contains(cid)) {
    return !m_channels.at(cid).empty();
  }
  return false;
}

net::Packet sim::details::Transport::recv(ChannelId cid) {
  // define the variable before assignment to silence a bogus
  // maybe-uninitialized error by GCC.
  PktOrIdx pkt_or_idx;

  pkt_or_idx = std::move(m_channels.at(cid).front());
  m_channels[cid].pop_front();

  if (pkt_or_idx.index() == 0) {
    // packet that was directly moved to us.
    return std::get<net::Packet>(pkt_or_idx);
  }

  const std::size_t idx = std::get<std::size_t>(pkt_or_idx);

  if (m_packets[idx].count == 0) {
    throw std::runtime_error("uh oh");
  }

  m_packets[idx].count--;
  return m_packets[idx].packet;
}

void sim::details::Transport::cleanUp(GlobalContext& ctx) {
  (void)ctx;
}
