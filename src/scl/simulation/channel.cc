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

#include "scl/coro.h"
#include "scl/simulation/context.h"
#include "scl/simulation/event.h"
#include "scl/simulation/transport.h"
#include "scl/time.h"

using namespace scl;

void details::SimulatedChannel::close() {
  if (!m_closed) {
    m_ctx.addEvent<CloseEvent>(m_ctx.elapsedTime(), m_id);
    m_closed = true;
  }
}

Task<void> details::SimulatedChannel::send(Packet&& packet) {
  const auto et = m_ctx.elapsedTime();
  m_ctx.addEvent<SendEvent>(et, m_id, packet.size());
  m_transport->send(et, m_id, std::move(packet));
  co_return;
}

Task<void> details::SimulatedChannel::send(const Packet& packet) {
  const auto et = m_ctx.elapsedTime();
  m_ctx.addEvent<SendEvent>(et, m_id, packet.size());
  m_transport->send(et, m_id, packet);
  co_return;
}

namespace {

// transient event signaling that this party is currently blocked while
// receiving.
class RecvPendingEvent final : public ChannelEvent {
 public:
  using ChannelEvent::ChannelEvent;

  void write(std::ostream&) override {}

  EventType type() const override {
    return EventType::TRANSIENT;
  }

  Time::Duration time() const override {
    return ChannelEvent::time() + m_offset;
  }

  void increaseOffset(Time::Duration t) {
    m_offset += t;
  }

 private:
  Time::Duration m_offset = Time::Duration::zero();
};

// Waits (i.e., suspends) until the transport has data ready for us
Task<void> waitForData(details::Transport* transport,
                       ChannelId id,
                       RecvPendingEvent* event,
                       details::Context& ctx) {
  while (!transport->ready(id)) {
    // If there's no data for us, then we can safely advance our clock ahead to
    // match the sender's.
    const auto diff = std::max(ctx.elapsedTimeOf(id.remote) - event->time(),
                               Time::Duration::zero());
    event->increaseOffset(diff);

    // Suspend this coroutine.
    co_await []() { return true; };
  }
}

}  // namespace

Task<Packet> details::SimulatedChannel::recv() {
  const auto et = m_ctx.elapsedTime();

  m_ctx.addEvent<RecvPendingEvent>(et, m_id);
  RecvPendingEvent* re = dynamic_cast<RecvPendingEvent*>(m_ctx.lastEvent());

  co_await waitForData(m_transport.get(), m_id, re, m_ctx);

  auto [pkt, delay] = m_transport->recv(re->time(), m_id);
  m_ctx.addEvent<RecvEvent>(re->time() + delay, m_id, pkt.size());

  co_return pkt;
}

namespace {

// Performs a similar action as waitForData, except that it is allowed to
// timeout. The return value indicates if a timeout happened or not. If no
// timeout happened, then it is assumed that data can be read from the
// transport, and that this data wasn't sent too far in the future.
Task<bool> waitOrTimeout(details::Transport* transport,
                         ChannelId id,
                         RecvPendingEvent* event,
                         details::Context& ctx,
                         Time::Duration timeout) {
  using namespace std::chrono_literals;
  const static auto timeout_wait_interval = 20ms;

  while (timeout >= Time::Duration::zero()) {
    const auto ready = transport->ready(id, event->time() + timeout);
    if (!ready) {
      const auto stime = ctx.elapsedTimeOf(id.remote);

      if (stime >= event->time()) {
        // sender_time >= our_time. Two cases, based on how far ahead the sender
        // is relative to us.
        //
        //          |---------- time ----------|
        //            |          |          |
        // case 1:    us      timeout     sender
        // case 2:    us      sender     timeout
        //
        // In the first case, we know that we're gonna timeout, so we can
        // advance our clock to the timeout mark, and return true.
        //
        // In the second case, we can advance our clock a little bit (the
        // timeout_wait_interval) and then suspend. We need to move our clock a
        // little bit to avoid deadlocks.

        if (stime > event->time() + timeout) {
          // case 1
          event->increaseOffset(timeout);
          co_return true;
        }

        // case 2
        event->increaseOffset(timeout_wait_interval);
        timeout -= timeout_wait_interval;
      }

      // Suspend this coroutine.
      co_await []() { return true; };
    }

    // there is data available within the timeout
    co_return false;
  }

  // timeout reached
  co_return true;
}

}  // namespace

Task<std::optional<Packet>> details::SimulatedChannel::recv(
    Time::Duration timeout) {
  const auto et = m_ctx.elapsedTime();

  m_ctx.addEvent<RecvPendingEvent>(et, m_id);
  RecvPendingEvent* re = dynamic_cast<RecvPendingEvent*>(m_ctx.lastEvent());

  bool timed_out =
      co_await waitOrTimeout(m_transport.get(), m_id, re, m_ctx, timeout);

  if (timed_out) {
    // we've timed out. womp womp.
    m_ctx.addEvent<RecvTimeoutEvent>(re->time(), m_id);
    co_return {};
  }

  auto [pkt, delay] = m_transport->recv(re->time(), m_id);
  m_ctx.addEvent<RecvEvent>(re->time() + delay, m_id, pkt.size());
  co_return pkt;
}

Task<bool> details::SimulatedChannel::poll() {
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
