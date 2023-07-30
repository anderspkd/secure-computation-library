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

#ifndef SCL_SIMULATION_SIMULATOR_H
#define SCL_SIMULATION_SIMULATOR_H

#include <algorithm>
#include <chrono>
#include <map>
#include <memory>
#include <queue>
#include <type_traits>
#include <utility>
#include <vector>

#include "scl/protocol/base.h"
#include "scl/simulation/config.h"
#include "scl/simulation/event.h"
#include "scl/simulation/manager.h"
#include "scl/simulation/result.h"

namespace scl::sim {

/**
 * @brief Exception used to signal that a simulation failed.
 *
 * In certain cases it is not possible to determine whether data is ready on a
 * channel (e.g., if the receiver is chronologically ahead of the sender). This
 * exception is used in these cases to "gracefully" interrupt the running party.
 */
struct SimulationFailure final : public std::runtime_error {
  /**
   * @brief Construct a new simulation failure exception.
   */
  SimulationFailure(const char* msg) : std::runtime_error(msg){};
  SimulationFailure() : SimulationFailure("simulation failed") {}
};

/**
 * @brief Compute the expected time that some bytes would be received.
 * @param config a simulation config, detailing the network conditions
 * @param n the number of bytes to receive
 * @return the time it took to send \p n bytes.
 *
 * This function is used throughout the simulation to compute how long it takes
 * for a number of bytes to arrive over the network used in the simulation. The
 * number of bytes is specified by the second argument \p n while the network
 * conditions (bandwidth, latency, overhead, etc...) is specified by \p config.
 */
util::Time::Duration ComputeRecvTime(const ChannelConfig& config,
                                     std::size_t n);

/**
 * @brief Simulate the execution of a protocol.
 * @param manager a simulation manager.
 * @return the simulation result.
 */
std::vector<Result> Simulate(std::unique_ptr<Manager> manager);

/**
 * @brief Simulate a protocol for a single replication.
 * @param protocol the protocol.
 * @return the simulation result.
 */
inline std::vector<Result> Simulate(
    std::vector<std::unique_ptr<proto::Protocol>> protocol) {
  return Simulate(
      std::make_unique<SingleReplicationManager>(std::move(protocol)));
}

}  // namespace scl::sim

#endif  // SCL_SIMULATION_SIMULATOR_H
