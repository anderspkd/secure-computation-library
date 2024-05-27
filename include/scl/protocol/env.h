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

#ifndef SCL_PROTOCOL_ENV_H
#define SCL_PROTOCOL_ENV_H

#include <chrono>
#include <memory>
#include <ratio>
#include <thread>
#include <utility>

#include "scl/net/network.h"
#include "scl/protocol/clock.h"
#include "scl/util/time.h"

namespace scl::proto {

/**
 * @brief Environment for protocol executions.
 *
 * ProtocolEnvironment is meant to provide a protocol controlled access to a
 * number of useful resources, such as a network, but also the ability to know
 * its total running time, as well as the ability to work with threads.
 */
struct Env {
  /**
   * @brief The network.
   */
  net::Network network;

  /**
   * @brief Clock used to tell for how long the protocol has been running.
   */
  std::unique_ptr<Clock> clock;
};

/**
 * @brief Create an environment from a network.
 * @param network the network.
 * @return an Env object.
 *
 * The returned environemnt uses the RealTimeClock and StlThreadContext for the
 * environment's clock and thread context, respectively.
 */
inline Env createDefaultEnv(net::Network network) {
  return Env{.network = std::move(network),
             .clock = std::make_unique<RealtimeClock>()};
}

}  // namespace scl::proto

#endif  // SCL_PROTOCOL_ENV_H
