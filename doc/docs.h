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

/*
 * This file serves only to add documentation to the different namespaces used
 * within SCL. It is also here that the different groups are defined.
 *
 * Documentation is structed according to the principle that, in order to run a
 * protocol, we need:
 *
 * 0. Some way to write a protocol.
 * 1. Some sort of network setup.
 * 2. Some sort of way to execute the protocol (given a network setup).
 *
 * Point (0) is all the boiler-plate functionality that is found in
 * - math/
 * - primitives/
 * - ss/
 * - bitmap, cmdline, hex, serialization and time
 *
 * Point (1) is anything related to networking.
 * - net/
 * - net/tcp/channel.h
 * - simulation/channel.h
 *
 * Point (2) is the simulation stuff, and the eval functions
 */

/**
 * @defgroup util Utilities
 * @brief Functionality that makes it easier to write MPC code
 */

/**
 * @defgroup ss Secret-sharing
 * @ingroup util
 * @brief Secret-sharing functionality
 */

/**
 * @defgroup math Maths
 * @ingroup util
 * @brief Math related utilities
 */

/**
 * @defgroup prim Cryptographic primitives
 * @ingroup util
 * @brief Cryptographic primitives
 */

/**
 * @defgroup net Networking
 * @brief Abstract network functionality.
 *
 * This group contains all SCL functionality related to networking, but which is
 * independent of the underlying transport protocol.
 */

/**
 * @defgroup net-tcp TCP
 * @ingroup net
 * @brief Networking functionality for working with TCP connections.
 */

/**
 * @defgroup net-sim Simulated
 * @ingroup net
 * @brief Networking functionality related to simulations.
 */

/**
 * @defgroup eval Protocols
 * @brief Utilities related to writing and running protocols.
 */

/**
 * @defgroup eval-real Real
 * @ingroup eval
 * @brief Evaluation of protocols with a real network.
 */

/**
 * @defgroup eval-sim Simulation
 * @ingroup eval
 * @brief Evaution of protocols in a simulated network environment.
 */

/**
 * @brief The main namespace.
 */
namespace scl {

/**
 * @brief Internal/low level namespace.
 */
namespace details {}

}  // namespace scl
