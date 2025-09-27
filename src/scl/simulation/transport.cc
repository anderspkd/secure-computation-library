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
#include "scl/simulation/network_description.h"

void scl::details::Transport::send(Time::Duration ts,
                                   ChannelId id,
                                   Packet&& packet) {
  const std::pair<Packet, Time::Duration> e{packet, ts};
  m_pqs[id].push(e);
}

void scl::details::Transport::send(Time::Duration ts,
                                   ChannelId id,
                                   const Packet& packet) {
  const std::pair<Packet, Time::Duration> e{packet, ts};
  m_pqs[id].push(e);
}

bool scl::details::Transport::ready(ChannelId id) const {
  // check if the other end of the channel contains anything.
  const auto sid = id.flip();
  return m_pqs.find(sid) != m_pqs.end() && !m_pqs.at(sid).empty();
}

namespace {

// These constants are somewhat arbitrary, yet sensibly, chosen. Should be good
// enough (?)
static constexpr std::size_t TCP_HEADER_SIZE_BYTES = 20;
static constexpr std::size_t IP_HEADER_SIZE_BYTES = 20;
static constexpr std::size_t MSS_BYTES = 1460;
static constexpr std::size_t WINDOW_SIZE_BYTES = 65535;

// computes the actual on-the-wire size when sending n bytes, by taking into
// account the size of a TCP/IP header, and the maximum segment size.
long double completeDataSize(std::size_t n) {
  const std::size_t packets = std::ceil((double)n / (double)MSS_BYTES);
  return 8 * (n + packets * (TCP_HEADER_SIZE_BYTES + IP_HEADER_SIZE_BYTES));
}

// utility function used to convert the latency value in a ChannelParameters
// struct to a long double.
long double rttSeconds(std::size_t latency_us) {
  using namespace std::chrono_literals;
  return std::chrono::microseconds(2 * latency_us) / 1.0s;
}

// Utility function that does the reverse of the above.
scl::Time::Duration convert(long double v) {
  const auto t0 = std::chrono::duration<long double>(v);
  return std::chrono::duration_cast<scl::Time::Duration>(t0);
}

// calculate the throughput of a channel.
long double throughput(scl::NetworkDescription::ChannelParameters params) {
  const auto rtt_secs = rttSeconds(params.latency);

  // all of these calculations are taken from "The Macroscopic Behavior of the
  // TCP Congestion Avoidance Algorithm" by Mathis, Semke and Mahdavi.
  //
  // https://cseweb.ucsd.edu/classes/wi01/cse222/papers/mathis-tcpmodel-ccr97.pdf

  long double tp;
  if (params.loss == 0) {
    tp = 8 * WINDOW_SIZE_BYTES / rtt_secs;
  } else {
    const long double C = std::sqrt(3.0 / 2.0);
    tp = 8 * MSS_BYTES / rtt_secs;
    tp *= C / std::sqrt(params.loss);
  }

  // whatever throughput we calculate cannot exceed the bandwidth of the
  // channel.
  return std::min(tp, (long double)params.bandwidth);
}

// compute the time it takes to send n bytes on a channel.
scl::Time::Duration recvTimeOffset(
    std::size_t n,
    scl::NetworkDescription::ChannelParameters params) {
  const long double total_size = completeDataSize(n);
  const long double tp = throughput(params);

  // the min here is needed in case we're sending very small amounts of
  // data. I.e., regardless of how good the channel is, we cannot send data
  // faster than the latency.
  const long double delay =
      std::min(total_size / tp, rttSeconds(params.latency));

  return convert(delay);
}

// adjust a receiver's timestamp based on (1) when the data was sent, (2) how
// much data was sent, and (3) the characteristics of the channel the data was
// sent on.
scl::Time::Duration computeDelay(
    scl::Time::Duration rt,
    scl::Time::Duration st,
    std::size_t n,
    scl::NetworkDescription::ChannelParameters params) {
  return std::max(st + recvTimeOffset(n, params) - rt,
                  scl::Time::Duration::zero());
}

}  // namespace

std::pair<scl::Packet, scl::Time::Duration> scl::details::Transport::recv(
    Time::Duration ts,
    ChannelId id) {
  const auto params = m_sim_ctx.getChannel(id);

  const auto sid = id.flip();
  const auto [pkt, t] = m_pqs[sid].front();
  m_pqs[sid].pop();

  const auto delay = computeDelay(ts, t, pkt.size(), params);

  return {pkt, delay};
}

namespace {

scl::Time::Duration smallestTimeDelta(
    scl::NetworkDescription::ChannelParameters params) {
  return recvTimeOffset(1, params);
}

}  // namespace

scl::details::Transport::PollResult scl::details::Transport::poll(
    Time::Duration ts,
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
