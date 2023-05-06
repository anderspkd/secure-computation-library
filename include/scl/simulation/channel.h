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

#ifndef SCL_SIMULATION_CHANNEL_H
#define SCL_SIMULATION_CHANNEL_H

#include <memory>

#include "scl/net/channel.h"
#include "scl/simulation/channel_id.h"
#include "scl/simulation/context.h"

namespace scl::sim {

/**
 * @brief Simulate a net::Channel::Close call on a channel.
 * @param ctx a simulation context.
 * @param id the ID of the channel making the call.
 * @return the event generated.
 *
 * This function simply generates a <code>CLOSE</code> event for the current
 * time of the running party.
 */
std::shared_ptr<Event> SimulateClose(std::shared_ptr<SimulationContext> ctx,
                                     ChannelId id);

/**
 * @brief Simulate a net::Channel::Send call on a channel.
 * @param ctx a simulation context.
 * @param id the ID of the channel making the call.
 * @param src a pointer to the data being sent.
 * @param n the number of bytes being sent.
 * @return the event generated.
 *
 * This function aims to simulate Sending data over a network, under the
 * assumption that this operation is instant. This involves generating a
 * <code>SEND</code> event with the current time of the party, minus the time it
 * took to write the data in \p src unto the underlying ChannelBuffer. In order
 * to determine when the \p n bytes are going to be received, this function also
 * records a write operation on the context with the time in the
 * <code>SEND</code> event for the number \p n of bytes sent.
 */
std::shared_ptr<Event> SimulateSend(std::shared_ptr<SimulationContext> ctx,
                                    ChannelId id,
                                    const unsigned char* src,
                                    std::size_t n);

/**
 * @brief Simulate a net::Channel::Recv call on a channel.
 * @param ctx a simulation context.
 * @param id the ID of the channel making the call.
 * @param dst destination for the received data.
 * @param n the number of bytes to receive.
 * @return the event generated.
 * @throws scl::SimulationFailure in case the call could not be simulated
 *
 * <p>This function fails in the case when less than \p n bytes are available on
 * the underlying ChannelBuffer. Otherwise, the function reads the requested
 * number of bytes, and computes the time this data would be received. Finally,
 * the function creates a <code>RECV</code> event with the adjusted time.
 *
 * <p>The time in the <code>RECV</code> event is adjusted by going through the
 * recorded write operations for the sending channel
 */
std::shared_ptr<Event> SimulateRecv(std::shared_ptr<SimulationContext> ctx,
                                    ChannelId id,
                                    unsigned char* dst,
                                    std::size_t n);

/**
 * @brief Simulate a net::Channel::HasData call on a channel.
 * @param ctx a simulation context
 * @param id the ID of the channel making the call
 * @return the event generated and the whether there was data available.
 * @throws scl::SimulationFailure in case the call could not be simulated
 *
 * <p>This function simulates the case where <code>id.local</code> checks if
 * <code>id.remote</code> sent it data. This is done by checking if there are
 * unhandled write operations by the remote party that took place before this
 * function was called.
 *
 * <p>This function may fail if the last time recorded by the remote party is
 * earlier than the time when this function was called. In this case, it is not
 * possible to determine if there are data available.
 */
std::pair<bool, std::shared_ptr<Event>> SimulateHasData(
    std::shared_ptr<SimulationContext> ctx,
    ChannelId id);

/**
 * @brief Channel implementation used in simulations.
 *
 * SimulatedChannel wraps a SimulationContext and a ChannelId and calls out to
 * sim::SimulateClose, sim::SimulateSend, sim::SimulateRecv or
 * sim::SimulateHasData, which performs the actual simulation of the methods in
 * the Channel interface.
 */
class SimulatedChannel final : public net::Channel {
 public:
  /**
   * @brief Construct a new Channel for simulations.
   * @param id the ID of the channel
   * @param ctx a simulation context object
   */
  SimulatedChannel(ChannelId id, std::shared_ptr<SimulationContext> ctx)
      : m_id(id), m_ctx(ctx){};

  void Close() override {
    m_ctx->AddEvent(m_id.local, SimulateClose(m_ctx, m_id));
  }

  void Send(const unsigned char* src, std::size_t n) override {
    m_ctx->AddEvent(m_id.local, SimulateSend(m_ctx, m_id, src, n));
  }

  std::size_t Recv(unsigned char* dst, std::size_t n) override {
    m_ctx->AddEvent(m_id.local, SimulateRecv(m_ctx, m_id, dst, n));
    return n;
  }

  bool HasData() override {
    const auto r = SimulateHasData(m_ctx, m_id);
    m_ctx->AddEvent(m_id.local, std::get<1>(r));
    return std::get<0>(r);
  }

  void Send(const net::Packet& packet) override;

  std::optional<net::Packet> Recv(bool block = true) override;

 private:
  ChannelId m_id;
  std::shared_ptr<SimulationContext> m_ctx;
};

}  // namespace scl::sim

#endif  // SCL_SIMULATION_CHANNEL_H
