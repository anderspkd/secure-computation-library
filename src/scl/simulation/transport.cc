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

#include "scl/simulation/transport.h"

#include <chrono>
#include <cmath>

#include "scl/simulation/channel_id.h"
#include "scl/simulation/context.h"
#include "scl/simulation/event.h"
#include "scl/simulation/params.h"

using namespace scl;

void details::Transport::send(Time::Duration ts,
                              ChannelId id,
                              Packet&& packet) {
  const std::pair<Packet, Time::Duration> e{packet, ts};
  m_pqs[id].push(e);
}

void details::Transport::send(Time::Duration ts,
                              ChannelId id,
                              const Packet& packet) {
  const std::pair<Packet, Time::Duration> e{packet, ts};
  m_pqs[id].push(e);
}

bool details::Transport::ready(ChannelId id) const {
  // check if the other end of the channel contains anything.
  const auto sid = id.flip();
  return m_pqs.find(sid) != m_pqs.end() && !m_pqs.at(sid).empty();
}

bool details::Transport::ready(ChannelId id, Time::Duration limit) const {
  const auto sid = id.flip();
  if (ready(id)) {
    return m_pqs.at(sid).front().second < limit;
  }
  return false;
}

namespace {

// These constants are somewhat arbitrary, yet sensibly, chosen. Should be good
// enough (?)
static constexpr std::size_t TCP_HEADER_SIZE_BYTES = 20;
static constexpr std::size_t IP_HEADER_SIZE_BYTES = 20;
static constexpr std::size_t MSS_BYTES = 1460;
static constexpr std::size_t WINDOW_SIZE_BYTES = 65535;

// computes the actual on-the-wire size in bits when sending n bytes, by taking
// into account the size of a TCP/IP header, and the maximum segment size.
long double completeDataSizeBits(std::size_t n) {
  const std::size_t packets = std::ceil((double)n / (double)MSS_BYTES);
  return 8 * (n + packets * (TCP_HEADER_SIZE_BYTES + IP_HEADER_SIZE_BYTES));
}

// utility function used to convert the latency value in a ChannelParameters
// struct to a long double.
long double rttSeconds(std::size_t latency_us) {
  using namespace std::chrono_literals;
  return (std::chrono::microseconds(2 * latency_us) / 1.0s);
}

// calculate the throughput of a channel.
long double throughput(std::size_t bw, std::size_t lat, float pl) {
  const auto rtt_secs = rttSeconds(lat);

  // all of these calculations are taken from "The Macroscopic Behavior of the
  // TCP Congestion Avoidance Algorithm" by Mathis, Semke and Mahdavi.
  //
  // https://cseweb.ucsd.edu/classes/wi01/cse222/papers/mathis-tcpmodel-ccr97.pdf

  long double tp;
  if (pl == 0) {
    tp = 8 * WINDOW_SIZE_BYTES / rtt_secs;
  } else {
    const long double C = std::sqrt(3.0 / 2.0);
    tp = 8 * MSS_BYTES / rtt_secs;
    tp *= C / std::sqrt(pl);
  }

  // whatever throughput we calculate cannot exceed the bandwidth of the
  // channel.
  return std::min(tp, (long double)bw);
}

// Utility function that does the reverse of the above.
Time::Duration convert(long double v) {
  const auto t0 = std::chrono::duration<long double>(v);
  return std::chrono::duration_cast<Time::Duration>(t0);
}

// compute the time it takes to send n bytes on a channel.
Time::Duration recvTimeOffset(std::size_t n, ChannelParams params) {
  const auto bw = params.bandwidth();
  const auto lat = params.latency();
  const auto pl = params.packetLoss();
  const long double total_size = completeDataSizeBits(n);
  const long double tp = throughput(bw, lat, pl);
  const long double delay = rttSeconds(lat) + total_size / tp;

  return convert(delay);
}

// adjust a receiver's timestamp based on (1) when the data was sent, (2) how
// much data was sent, and (3) the characteristics of the channel the data was
// sent on.
Time::Duration computeDelay(Time::Duration rt,
                            Time::Duration st,
                            std::size_t n,
                            ChannelParams params) {
  const auto offset = recvTimeOffset(n, params);
  const auto x = st + offset - rt;
  return std::max(x, Time::Duration::zero());
}

}  // namespace

std::pair<Packet, Time::Duration> details::Transport::recv(Time::Duration ts,
                                                           ChannelId id) {
  const auto params = m_sim_ctx.getChannel(id);

  const auto sid = id.flip();
  const auto [pkt, t] = m_pqs[sid].front();
  m_pqs[sid].pop();

  const auto delay = computeDelay(ts, t, pkt.size(), params);

  return {pkt, delay};
}

namespace {

Time::Duration smallestTimeDelta(ChannelParams params) {
  return recvTimeOffset(1, params);
}

}  // namespace

details::Transport::PollResult details::Transport::poll(Time::Duration ts,
                                                        ChannelId id) const {
  const auto stime = m_sim_ctx.getContext(id.remote).lastEvent()->time();
  const auto delta = smallestTimeDelta(m_sim_ctx.getChannel(id));

  // we split the logic into two branches, based on whether the sender's local
  // time is behind the caller's (i.e., the receiver). We offset the comparison
  // with the time it takes to send a single byte since, what really matters, is
  // whether the sender could have sent something that we would see by now.
  if (stime + delta < ts) {
    // sender is behind us, but have already sent us something. In that case,
    // we'd definitely be able to receive.
    if (ready(id)) {
      return Transport::PollResult::DATA;
    }

    // sender has not sent us anything, but they might! However, we can be sure
    // that they are not sending us anything if (1) they are waiting for us, and
    // (2) we have not sent them anything. In this case, the sender will not
    // send us anything before we send them something. In particular, there's no
    // data coming to us right now.
    return Transport::PollResult::NA;

  } else {
    // sender is ahead of us, but didn't send us anything. We can reliably say
    // that there's no data in this case.
    if (!ready(id)) {
      return Transport::PollResult::NO_DATA;
    }

    // sender is ahead of us, and sent us something. Whether we'd be able to see
    // this data depends on when the data was sent. If it was sent in the past
    // (from our pov), then we'd be able to receive it. Otherwise we won't.
    const auto pkt_time = m_pqs.at(id).back().second;
    if (pkt_time + delta <= ts) {
      return Transport::PollResult::DATA;
    } else {
      return Transport::PollResult::NO_DATA;
    }
  }
}
