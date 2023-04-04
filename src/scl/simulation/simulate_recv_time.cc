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

#include <chrono>
#include <cmath>
#include <ratio>

#include "scl/simulation/config.h"
#include "scl/simulation/simulator.h"

namespace {

/**
 * @brief Computes the actual number of bytes needing transfer
 * @param nbytes the raw number of bytes
 * @param mss the maximum segment size
 * @param the number of bytes when accounting for overhead.
 *
 * This function computes the number of packages needed to send \p nbytes worth
 * of data and adds an overhead of 40 bytes per package.
 */
long double TransferSizeWithHeadersBits(std::size_t nbytes,
                                        std::size_t mss) noexcept {
  static constexpr std::size_t kHeaderSizeBytes = 40;
  const std::size_t num_packets = std::ceil((double)nbytes / (double)mss);
  return 8 * (nbytes + num_packets * kHeaderSizeBytes);
}

/**
 * @brief Get the RTT from a config in seconds.
 */
long double RoundTripTimeSeconds(
    const scl::sim::SimulatedNetworkConfig& config) noexcept {
  using namespace std::chrono_literals;
  const auto d = std::chrono::milliseconds(config.RTT());
  return d / 1.0s;
}

/**
 * @brief Compute the maximum TCP throughput assuming package loss of 0%
 */
long double ThroughputZeroPackageLoss(
    const scl::sim::SimulatedNetworkConfig& config) noexcept {
  // Simple throughput formula:
  // https://tetcos.com/pdf/v13/Experiments/Mathematical-Modelling-of-TCP-Throughput-Performance.pdf
  const auto rtt = RoundTripTimeSeconds(config);
  const auto wndz = 8 * (long double)config.WindowSize();
  const auto max_throughput = wndz / rtt;

  // actual throughput obviously cannot exceed the capacity of the link.
  const auto bw = (long double)config.Bandwidth();
  const auto actual_throughput = std::min(max_throughput, bw);

  return actual_throughput;
}

/**
 * @brief Compute TCP throughput assuming package loss using Mathis et. al.
 */
long double ThroughputNonZeroPackageLoss(
    const scl::sim::SimulatedNetworkConfig& config) noexcept {
  const auto mss = (long double)config.MSS();
  const auto loss_term = std::sqrt(3.0 / (2.0 * config.PackageLoss()));
  const auto rtt = RoundTripTimeSeconds(config);

  return loss_term * (8 * mss / rtt);
}

}  // namespace

scl::util::Time::Duration scl::sim::ComputeRecvTime(
    const SimulatedNetworkConfig& config,
    std::size_t n) {
  const auto total_size_bits = TransferSizeWithHeadersBits(n, config.MSS());
  auto actual_tp = ThroughputZeroPackageLoss(config);

  if (config.PackageLoss() > 0) {
    const auto tp = ThroughputNonZeroPackageLoss(config);
    actual_tp = std::min(tp, actual_tp);
  }

  const auto t = total_size_bits / actual_tp + RoundTripTimeSeconds(config);
  const auto t_sec = std::chrono::duration<double>(t);
  return std::chrono::duration_cast<util::Time::Duration>(t_sec);
}
