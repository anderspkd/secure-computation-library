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
#include <vector>

#include "scl/protocol/base.h"
#include "scl/simulation/config.h"
#include "scl/simulation/event.h"
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
util::Time::Duration ComputeRecvTime(const SimulatedNetworkConfig& config,
                                     std::size_t n);

/**
 * @brief Protocol creator interface.
 *
 * A ProtocolCreator is a supplier of the protocol that is being simulated.
 * Simulations need to be run multiple times in order to get good measurements.
 * This interface captures a type whose only job is to return a <i>fresh</i>
 * protocol definition every time it is called.
 */
using ProtocolCreator =
    std::function<std::vector<std::unique_ptr<proto::Protocol>>()>;

/**
 * @brief Callback function types when a party creates an output.
 *
 * Whenever a party produces output during a simulation, a callback of this type
 * is called with the party's ID as the first argument, and the output produced
 * as the second.
 */
using OutputCallback = std::function<void(std::size_t, const std::any&)>;

/**
 * @brief Simulate a protocol execution.
 * @param protocol_creator a creator object for the protocol being simulated
 * @param config_creator a simulation config creator object
 * @param iterations how many iterations the simulation should run for
 * @param output_cb a function that is called when a party produce an output
 * @return the simulation result.
 */
std::vector<Result> Simulate(
    const ProtocolCreator& protocol_creator,
    const SimulatedNetworkConfigCreator& config_creator,
    std::size_t iterations,
    const OutputCallback& output_cb);

/**
 * @brief Simulate a protocol execution.
 * @param parties the parties of the protocol
 * @param config_creator a simulation config creator object
 * @param output_cb a function that is called when a party produce an output
 * @return the simulation result.
 */
std::vector<Result> Simulate(
    std::vector<std::unique_ptr<proto::Protocol>> parties,
    const SimulatedNetworkConfigCreator& config_creator,
    const OutputCallback& output_cb);

/**
 * @brief Simulate a protocol execution.
 * @param protocol_creator a creator object for the protocol being simulated
 * @param config_creator a simulation config creator object
 * @param iterations the number of iterations to run the simulation for
 */
inline std::vector<Result> Simulate(
    const ProtocolCreator& protocol_creator,
    const SimulatedNetworkConfigCreator& config_creator,
    std::size_t iterations) {
  const auto cb = [](auto id, auto output) {
    (void)id;
    (void)output;
  };
  return Simulate(protocol_creator, config_creator, iterations, cb);
}

/**
 * @brief Simulate a protocol execution.
 * @param parties the parties of the protocol to simulate
 * @param config_creator a simulation config creator object
 */
inline std::vector<Result> Simulate(
    std::vector<std::unique_ptr<proto::Protocol>> parties,
    const SimulatedNetworkConfigCreator& config_creator) {
  const auto cb = [](auto id, auto output) {
    (void)id;
    (void)output;
  };
  return Simulate(std::move(parties), config_creator, cb);
}

}  // namespace scl::sim

#endif  // SCL_SIMULATION_SIMULATOR_H
