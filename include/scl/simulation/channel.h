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

#ifndef SCL_SIMULATION_CHANNEL_H
#define SCL_SIMULATION_CHANNEL_H

#include <memory>

#include "scl/coro/task.h"
#include "scl/net/channel.h"
#include "scl/simulation/channel_id.h"
#include "scl/simulation/context.h"
#include "scl/simulation/transport.h"

namespace scl::sim::details {

/**
 * @brief Channel implementation used during simulations.
 */
class SimulatedChannel final : public net::Channel {
 public:
  /**
   * @brief Construct a SimulatedChannel.
   * @param cid the ID of this channel.
   * @param context a context object for this channel.
   * @param transport the transport to use for moving data.
   */
  SimulatedChannel(ChannelId cid,
                   GlobalContext::LocalContext context,
                   std::shared_ptr<Transport> transport)
      : m_cid(cid), m_context(context), m_transport(transport) {}

  /**
   * @brief Closes the channel.
   *
   * Creates a EventType::CLOSE event.
   */
  void close() override;

  /**
   * @brief Sends data on the channel.
   *
   * Creates a EventType::SEND event.
   */
  coro::Task<void> send(net::Packet&& packet) override;

  /**
   * @brief Sends data on the channel.
   *
   * Creates a EventType::SEND event.
   */
  coro::Task<void> send(const net::Packet& packet) override;

  /**
   * @brief Receives data on the channel.
   *
   * Creates a EventType::RECV event.
   */
  coro::Task<net::Packet> recv() override;

  /**
   * @brief Checks if there is data available on this channel.
   *
   * Creates a EventType::HAS_DATA event.
   */
  coro::Task<bool> hasData() override;

 private:
  ChannelId m_cid;
  GlobalContext::LocalContext m_context;
  std::shared_ptr<Transport> m_transport;
};

}  // namespace scl::sim::details

#endif  // SCL_SIMULATION_CHANNEL_H
