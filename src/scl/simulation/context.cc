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

#include "scl/simulation/context.h"

#include <chrono>
#include <cmath>
#include <ratio>

#include "scl/simulation/config.h"
#include "scl/simulation/event.h"
#include "scl/simulation/hook.h"
#include "scl/util/bitmap.h"

using namespace scl;
// makes things a bit nicer to read.
using GlobalCtx = sim::details::GlobalContext;

namespace {

std::vector<util::Bitmap> initBitmaps(std::size_t number_of_parties) {
  std::vector<util::Bitmap> bms;
  bms.reserve(number_of_parties);
  for (std::size_t i = 0; i < number_of_parties; i++) {
    bms.emplace_back(number_of_parties);
  }
  return bms;
}

}  // namespace

GlobalCtx GlobalCtx::create(std::size_t number_of_parties,
                            std::unique_ptr<NetworkConfig> network_config,
                            std::vector<TriggerAndHook> hooks) {
  std::vector<sim::SimulationTrace> traces(number_of_parties);

  for (std::size_t i = 0; i < number_of_parties; i++) {
    traces.reserve(1024);
  }

  std::unordered_map<sim::ChannelId, std::deque<util::Time::Duration>> sends;
  std::vector<util::Time::TimePoint> clocks(number_of_parties);
  std::vector<util::Bitmap> recv_map = initBitmaps(number_of_parties);

  return {number_of_parties,
          std::move(network_config),
          traces,
          sends,
          clocks,
          recv_map,
          util::Bitmap(number_of_parties),
          std::move(hooks)};
}

util::Time::Duration GlobalCtx::LocalContext::lastEventTimestamp() const {
  if (!m_gctx.traces[m_id].empty()) {
    return m_gctx.traces[m_id].back()->timestamp;
  }
  return util::Time::Duration::zero();
}

util::Time::Duration GlobalCtx::LocalContext::elapsedTime() const {
  const util::Time::Duration most_recent = lastEventTimestamp();
  return most_recent + (util::Time::now() - m_gctx.clocks[m_id]);
}

void GlobalCtx::LocalContext::startClock() {
  m_gctx.clocks[m_id] = util::Time::now();
}

namespace {

// Computes total size in bits that nbytes of data would occupy provided some
// maximum segment size
long double sizeWithHeadersInBits(std::size_t nbytes,
                                  std::size_t mss) noexcept {
  static constexpr std::size_t TCP_IP_HEADER = 40;
  const std::size_t num_packets = std::ceil((double)nbytes / (double)mss);
  return 8 * (nbytes + num_packets * TCP_IP_HEADER);
}

// Converts the RTT in a config, assumed to be in ms, to seconds.
long double rttSeconds(const sim::ChannelConfig& config) noexcept {
  using namespace std::chrono_literals;
  const auto d = std::chrono::milliseconds(config.RTT());
  return d / 1.0s;
}

// Computes the throughput of a channel assuming a package loss of 0%.
long double throughputNoLoss(const sim::ChannelConfig& config) noexcept {
  // Simple throughput formula:
  // https://tetcos.com/pdf/v13/Experiments/Mathematical-Modelling-of-TCP-Throughput-Performance.pdf
  const long double rtt = rttSeconds(config);
  const long double wndz = 8 * (long double)config.windowSize();
  const long double max_throughput = wndz / rtt;

  // actual throughput obviously cannot exceed the capacity of the link.
  const long double bw = (long double)config.bandwidth();
  const long double actual_throughput = std::min(max_throughput, bw);

  return actual_throughput;
}

// Computes the throughput of a channel assuming a package loss of > 0%. This
// uses the Mathis formula:
// https://cseweb.ucsd.edu/classes/wi01/cse222/papers/mathis-tcpmodel-ccr97.pdf
long double throughputLoss(const sim::ChannelConfig& config) noexcept {
  const long double mss = config.MSS();
  const long double loss_term = std::sqrt(3.0 / (2.0 * config.packetLoss()));
  const long double rtt = rttSeconds(config);

  return loss_term * (8 * mss / rtt);
}

// Computes the receive time of some amount of data on a TCP channel.
util::Time::Duration recvTimeTCP(const sim::ChannelConfig& config,
                                 std::size_t n) {
  const long double total_size_bits = sizeWithHeadersInBits(n, config.MSS());
  long double actual_tp = throughputNoLoss(config);

  if (config.packetLoss() > 0) {
    const long double tp = throughputLoss(config);
    actual_tp = std::min(tp, actual_tp);
  }

  const long double t = total_size_bits / actual_tp + rttSeconds(config);
  const auto t_sec = std::chrono::duration<double>(t);
  return std::chrono::duration_cast<util::Time::Duration>(t_sec);
}

// Computes the delay that sending an amount of bytes would incur.
util::Time::Duration adjustSendTime(const sim::ChannelConfig& config,
                                    util::Time::Duration send_time,
                                    std::size_t n) {
  if (config.type() == sim::ChannelConfig::NetworkType::TCP) {
    return send_time + recvTimeTCP(config, n);
  }
  return send_time;
}

}  // namespace

void GlobalCtx::LocalContext::recordEvent(std::shared_ptr<Event> event) {
  m_gctx.traces[m_id].emplace_back(event);

  const auto event_type = event->type;
  for (const auto& [trigger, hook] : m_gctx.hooks) {
    if (trigger.has_value()) {
      if (trigger.value() == event_type) {
        hook->run(m_id, getContext());
      }
    } else {
      hook->run(m_id, getContext());
    }
  }
}

util::Time::Duration GlobalCtx::LocalContext::recv(
    std::size_t sender,
    std::size_t nbytes,
    util::Time::Duration timestamp) {
  // Channel ID corresponding to the channel that the remote party writes to.
  const ChannelId id{.local = sender, .remote = m_id};
  const util::Time::Duration send_time = m_gctx.sends[id].front();
  m_gctx.sends[id].pop_front();

  const ChannelConfig cconf = m_gctx.network_config->get(id);
  return std::max(timestamp, adjustSendTime(cconf, send_time, nbytes));
}

void GlobalCtx::LocalContext::recvStart(std::size_t id) {
  m_gctx.recv_map[m_id].set(id, true);
}

void GlobalCtx::LocalContext::recvDone(std::size_t id) {
  m_gctx.recv_map[m_id].set(id, false);
}

bool GlobalCtx::LocalContext::receiving(std::size_t receiver) const {
  return m_gctx.recv_map[receiver].at(m_id);
}

bool GlobalCtx::LocalContext::dead(std::size_t id) const {
  if (m_gctx.traces[id].empty()) {
    return false;
  }

  const auto last_event_type = m_gctx.traces[id].back()->type;
  return last_event_type == EventType::STOP ||
         last_event_type == EventType::KILLED ||
         last_event_type == EventType::CANCELLED;
}

util::Time::Duration GlobalCtx::LocalContext::currentTimeOf(
    std::size_t other_party) const {
  if (m_gctx.traces[other_party].empty()) {
    return util::Time::Duration::zero();
  }
  return m_gctx.traces[other_party].back()->timestamp;
}

std::ostream& sim::details::operator<<(
    std::ostream& os,
    const sim::details::GlobalContext& global_ctx) {
  os << "GLOBAL_CTX{";
  os << " number_of_parties=" << global_ctx.number_of_parties << "\n";
  os << " network_config=<omitted>\n";
  os << " traces=<omitted>\n";
  os << " sends=<omitted>\n";
  os << " clocks=<omitted>\n";
  os << " recv_map=<omitted>\n";
  os << " cancellation_map=" << global_ctx.cancellation_map << "\n";
  os << " hooks=<omitted>\n";
  os << "}\n";

  return os;
}
