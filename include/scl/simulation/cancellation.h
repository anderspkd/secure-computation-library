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

#ifndef SCL_SIMULATION_CANCELLATION_H
#define SCL_SIMULATION_CANCELLATION_H

#include <cstddef>
#include <optional>
#include <stdexcept>

#include "scl/util/bitmap.h"

namespace scl::sim::details {

/**
 * @brief Exception used to signal that a coroutine has been cancelled.
 */
struct CancellationException final : public std::runtime_error {
  CancellationException() : std::runtime_error("cancelled") {}
};

}  // namespace scl::sim::details

#endif  // SCL_SIMULATION_CANCELLATION_H
