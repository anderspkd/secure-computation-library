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

#include "scl/simulation/channel.h"

#include "scl/coro/runtime.h"
#include "scl/simulation/event.h"
#include "scl/simulation/transport.h"
#include "scl/time.h"

void scl::SimulatedChannel::close() {
  m_ctx.addEvent<scl::CloseEvent>(m_ctx.elapsedTime(), m_id);
}

scl::Task<void> scl::SimulatedChannel::send(Packet&& packet) {
  const auto et = m_ctx.elapsedTime();
  m_ctx.addEvent<SendEvent>(et, m_id, packet.size());
  m_transport->send(et, m_id, std::move(packet));
  co_return;
}

scl::Task<void> scl::SimulatedChannel::send(const Packet& packet) {
  const auto et = m_ctx.elapsedTime();
  m_ctx.addEvent<SendEvent>(et, m_id, packet.size());
  m_transport->send(et, m_id, packet);
  co_return;
}

namespace {

// transient event signaling that this party is currently blocked while
// receiving.
class RecvPendingEvent final : public scl::ChannelEvent {
 public:
  RecvPendingEvent(scl::Time::Duration timestamp, scl::ChannelId id)
      : ChannelEvent(timestamp, id), m_offset(scl::Time::Duration::zero()) {}

  void write(std::ostream&) override {}

  scl::EventType type() const override {
    return scl::EventType::TRANSIENT;
  }

  scl::Time::Duration time() const override {
    return scl::ChannelEvent::time() + m_offset;
  }

  void bumpOffset(scl::Time::Duration t) {
    if (t > m_offset) {
      m_offset = t;
    }
  }

 private:
  scl::Time::Duration m_offset;
};

// Awaitable that checks if the transport is ready.
struct ReadyChecker {
  scl::Transport* transport;
  scl::ChannelId id;
  RecvPendingEvent* event;
  scl::Context& ctx;

  bool operator()() {
    const auto is_ready = transport->ready(id);
    if (!is_ready) {
      // transport not being ready means the sender hasn't sent anything
      // yet. This also means that anything that does get sent, wont get sent
      // before whatever the time is at the sender. And so we can safely advance
      // our clock.
      event->bumpOffset(ctx.elapsedTimeOf(id.remote));
    }
    return is_ready;
  }
};

}  // namespace

scl::Task<scl::Packet> scl::SimulatedChannel::recv() {
  const auto et = m_ctx.elapsedTime();

  m_ctx.addEvent<RecvPendingEvent>(et, m_id);
  RecvPendingEvent* re = dynamic_cast<RecvPendingEvent*>(m_ctx.lastEvent());

  ReadyChecker rc{m_transport.get(), m_id, re, m_ctx};
  co_await rc;

  auto [pkt, delay] = m_transport->recv(re->time(), m_id);

  m_ctx.addEvent<RecvEvent>(re->time() + delay, m_id, pkt.size());

  co_return pkt;
}

namespace {

// Ready checker which capable of timing out
struct TimeoutReadyChecker {
  scl::Transport* transport;
  scl::ChannelId id;
  RecvPendingEvent* event;
  scl::Context& ctx;
  scl::Time::Duration timeout_rem;

  bool operator()() {
    const auto t = ctx.elapsedTimeOf(id.remote);
    if (event->time() > t) {
      return transport->ready(id);
    }

    const auto is_ready = transport->ready(id);
    if (!is_ready) {
      // Since the transport is not ready, we can safely advance our time to the
      // time of the sender.
      event->bumpOffset(ctx.elapsedTimeOf(id.remote));
    }
    return is_ready;
  }
};

}  // namespace

scl::Task<std::optional<scl::Packet>> scl::SimulatedChannel::recv(
    Time::Duration timeout) {
  // const auto et = m_ctx.elapsedTime();

  // m_ctx.addEvent<RecvPendingEvent>(et, m_id);
  // RecvPendingEvent* re = dynamic_cast<RecvPendingEvent*>(m_ctx.lastEvent());

  // TimeoutReadyChecker trc{m_transport.get(), m_id, re, m_ctx, timeout};
  // co_await trc;

  (void)timeout;
  co_return {};
}

scl::Task<bool> scl::SimulatedChannel::poll() {
  const auto et = m_ctx.elapsedTime();
  Transport::PollResult pr;

  co_await [&pr, e = et, i = m_id, t = m_transport]() {
    pr = t->poll(e, i);
    return pr != Transport::PollResult::NA;
  };

  const auto res = pr == Transport::PollResult::DATA;
  m_ctx.addEvent<PollEvent>(et, m_id, res);

  co_return res;
}
