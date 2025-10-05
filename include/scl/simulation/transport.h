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

#include <queue>
#include <unordered_map>

#include "scl/net/packet.h"
#include "scl/simulation/channel_id.h"
#include "scl/simulation/context.h"
#include "scl/time.h"

namespace scl::details {

/**
 * @brief Handles the movement of packets between parties in a simulation.
 * @ingroup ss
 */
class Transport {
 public:
  Transport(SimulatorContext& sim_ctx) : m_sim_ctx(sim_ctx) {}

  /**
   * @brief Queue data for sending.
   */
  void send(Time::Duration ts, ChannelId id, const Packet& pkt);

  /**
   * @brief Queue data for sending.
   */
  void send(Time::Duration ts, ChannelId id, Packet&& pkt);

  /**
   * @brief Check if there is data available for receiving.
   */
  bool ready(ChannelId id) const;

  /**
   * @brief Check if there is data available for receiving within a timelimit.
   */
  bool ready(ChannelId id, Time::Duration limit) const;

  /**
   * @brief Receive data.
   */
  std::pair<Packet, Time::Duration> recv(Time::Duration ts, ChannelId id);

  enum class PollResult { NA, DATA, NO_DATA };

  /**
   * @brief Check if it is possible to receive data at
   */
  PollResult poll(Time::Duration ts, ChannelId id) const;

 private:
  SimulatorContext& m_sim_ctx;
  std::unordered_map<ChannelId, std::queue<std::pair<Packet, Time::Duration>>>
      m_pqs;
};

}  // namespace scl::details
