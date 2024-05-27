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

#include "scl/simulation/channel.h"

#include <stdexcept>

#include "scl/coro/runtime.h"
#include "scl/simulation/event.h"
#include "scl/util/time.h"

using namespace scl;

namespace {

std::size_t totalPacketSize(const net::Packet& packet) {
  return packet.size() + sizeof(net::Packet::SizeType);
}

}  // namespace

void sim::details::SimulatedChannel::close() {
  util::Time::Duration elapsed = m_context.elapsedTime();
  m_context.recordEvent(Event::closeChannel(elapsed, m_cid));
  m_context.startClock();
}

coro::Task<void> sim::details::SimulatedChannel::send(net::Packet&& packet) {
  util::Time::Duration elapsed = m_context.elapsedTime();
  const std::size_t nbytes = totalPacketSize(packet);
  m_context.send(m_cid.remote, elapsed);

  m_transport->send(m_cid, std::move(packet));

  m_context.recordEvent(Event::sendData(elapsed, m_cid, nbytes));
  m_context.startClock();
  co_return;
}

coro::Task<void> sim::details::SimulatedChannel::send(
    const net::Packet& packet) {
  util::Time::Duration elapsed = m_context.elapsedTime();
  const std::size_t nbytes = totalPacketSize(packet);
  m_context.send(m_cid.remote, elapsed);

  m_transport->send(m_cid, packet);

  m_context.recordEvent(Event::sendData(elapsed, m_cid, nbytes));
  m_context.startClock();
  co_return;
}

coro::Task<net::Packet> sim::details::SimulatedChannel::recv() {
  util::Time::Duration elapsed = m_context.elapsedTime();

  m_context.recvStart(m_cid.remote);

  // block until there is data available on the transport.
  co_await [tp = m_transport, cid = m_cid]() { return tp->hasData(cid); };

  auto packet = m_transport->recv(m_cid);

  m_context.recvDone(m_cid.remote);

  elapsed = m_context.recv(m_cid.remote, totalPacketSize(packet), elapsed);

  const std::size_t nbytes = totalPacketSize(packet);
  m_context.recordEvent(Event::recvData(elapsed, m_cid, nbytes));
  m_context.startClock();
  co_return packet;
}

coro::Task<bool> sim::details::SimulatedChannel::hasData() {
  util::Time::Duration now = m_context.elapsedTime();
  m_context.recordEvent(Event::hasData(now, m_cid));

  auto has_data = m_transport->hasData(m_cid);

  if (!has_data) {
    const auto other = m_cid.remote;

    // have to consider three cases here:
    //
    // 1) If the remote party is ahead of us, then any data it sends will
    // first
    //    arrive at some point in the future.
    //
    // 2) If the remote party is dead, then _no_ data will arrive to us.
    //
    // 3) If the remote party is trying to receive data from us, then it will
    //    not have data for us until we send something, which cannot be
    //    earlier than "now". In particular, we won't receive data from remote
    //    until at some point after whatever "now" is.
    co_await [now, ctx = m_context, other]() {
      const auto remote_ahead = now < ctx.currentTimeOf(other);
      const auto remote_dead = ctx.dead(other);
      const auto remote_waiting_for_us = ctx.receiving(other);

      return remote_ahead || remote_dead || remote_waiting_for_us;
    };

    m_context.startClock();
    // query the transport again.
    co_return m_transport->hasData(m_cid);
  }

  m_context.startClock();
  co_return true;
}
