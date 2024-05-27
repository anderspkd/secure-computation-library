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

#ifndef SCL_SIMULATION_SIMULATOR_H
#define SCL_SIMULATION_SIMULATOR_H

#include <memory>
#include <utility>
#include <vector>

#include "scl/protocol/base.h"
#include "scl/simulation/config.h"
#include "scl/simulation/event.h"
#include "scl/simulation/manager.h"

namespace scl::sim {

/**
 * @brief Simulate the execution of a protocol.
 * @param manager a simulation manager.
 */
void simulate(std::unique_ptr<Manager> manager);

}  // namespace scl::sim

#endif  // SCL_SIMULATION_SIMULATOR_H
