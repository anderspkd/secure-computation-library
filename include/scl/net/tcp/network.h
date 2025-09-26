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

#ifndef SCL_NET_TCP_NETWORK_H
#define SCL_NET_TCP_NETWORK_H

#include "scl/coro/task.h"
#include "scl/net/config.h"
#include "scl/net/network.h"

namespace scl {

/**
 * @brief Create a network using a network config.
 * @param config the network configuration to use.
 *
 * Creates a new network where the connection information about the parties of
 * the network is read from a provided config. In the resulting network, the
 * local party is connected to itself with a LoopbackChannel, and to everyone
 * else with a TcpChannel.
 */
Task<Network> createTcpNetwork(const NetworkConfig& config);

}  // namespace scl

#endif  // SCL_NET_TCP_NETWORK_H
