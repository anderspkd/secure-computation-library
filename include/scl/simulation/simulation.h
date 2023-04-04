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

#ifndef SCL_SIMULATION_SIMULATION_H
#define SCL_SIMULATION_SIMULATION_H

#include "scl/simulation/config.h"
#include "scl/simulation/result.h"
#include "scl/simulation/simulator.h"

/**
 * @brief Utilities for simulating protocol executions.
 *
 * <p>SCL supports running a protocol, as defined via. the proto::Protocol
 * interface, to be run using both a real network, as well as a simulated one.
 * This will allow the user of SCL to implement a protocol <i>once</i> and then
 * run it under either a real network, where all the parties are connected
 * through pair-wise TCP channels, or a "fake" one, where parties are simply
 * emulated.
 *
 * <p>Let's consider a simple example.
 */
namespace scl::sim {}  // namespace scl::sim

#endif  // SCL_SIMULATION_SIMULATION_H
