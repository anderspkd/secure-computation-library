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

class SimulatedChannel final : public Channel {
 public:
  static std::shared_ptr<Channel> create(ChannelId id,
                                         Context ctx,
                                         std::shared_ptr<Transport> transport) {
    return std::make_shared<SimulatedChannel>(id, ctx, transport);
  }

  SimulatedChannel(ChannelId id,
                   Context ctx,
                   std::shared_ptr<Transport> transport)
      : m_id(id), m_ctx(ctx), m_transport(transport) {}

  void close() override;
  Task<void> send(Packet&& packet) override;
  Task<void> send(const Packet& packet) override;
  Task<Packet> recv() override;
  Task<std::optional<Packet>> recv(Time::Duration timeout) override;
  Task<bool> poll() override;

 private:
  ChannelId m_id;
  Context m_ctx;
  std::shared_ptr<Transport> m_transport;
};

}  // namespace scl::details
