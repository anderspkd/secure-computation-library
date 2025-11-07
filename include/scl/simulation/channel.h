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

#include "scl/net/channel.h"
#include "scl/net/packet.h"
#include "scl/simulation/channel_id.h"
#include "scl/simulation/context.h"
#include "scl/simulation/transport.h"

namespace scl::details {

/**
 * @brief Channel implementation for simulations.
 * @ingroup net-sim
 *
 * SimulatedChannel implements Channel for non-local parties in simulations. The
 * main responsibilities of this class are (1) calling a Transport object with
 * the stuff to be sent/received, and (2) generating the right Events.
 */
class SimulatedChannel final : public Channel {
 public:
  /**
   * @brief Create a new SimulatedChannel.
   */
  static std::shared_ptr<Channel> create(ChannelId id,
                                         Context ctx,
                                         std::shared_ptr<Transport> transport) {
    return std::make_shared<SimulatedChannel>(id, ctx, transport);
  }

  SimulatedChannel(ChannelId id,
                   Context ctx,
                   std::shared_ptr<Transport> transport)
      : m_id(id), m_ctx(ctx), m_transport(transport), m_closed(false) {}

  /**
   * @brief Closes the channel.
   */
  void close() override;

  /**
   * @brief Send a packet.
   */
  Task<void> send(Packet&& packet) override;

  /**
   * @brief Send a packet.
   */
  Task<void> send(const Packet& packet) override;

  /**
   * @brief Recv a packet.
   */
  Task<Packet> recv() override;

  /**
   * @brief Attempt to receive a packet within a provied timeout.
   */
  Task<std::optional<Packet>> recv(Time::Duration timeout) override;

  /**
   * @brief Poll the channel for data.
   */
  Task<bool> poll() override;

 private:
  ChannelId m_id;
  Context m_ctx;
  std::shared_ptr<Transport> m_transport;

  bool m_closed;
};

}  // namespace scl::details
