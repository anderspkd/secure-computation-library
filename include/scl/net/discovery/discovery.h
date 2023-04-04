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

#ifndef SCL_NET_DISCOVERY_DISCOVERY_H
#define SCL_NET_DISCOVERY_DISCOVERY_H

#include "scl/net/config.h"
#include "scl/net/discovery/client.h"
#include "scl/net/discovery/server.h"

namespace scl::net {

/**
 * @brief Class for running either a client or server for peer-discovery.
 *
 * <p>The two functions in Discovery can be used to discover other protocol
 * peers, such that an explicit config does not have to be provided to each
 * party at start-up.
 *
 * <p>Discovery works by letting one party run as the server, and everyone else
 * run as clients. The server will receive a hostname, IP and ID from each
 * party, create a NetworkConfig and send it back to all parties. Discovery can
 * therefore be run as a step before an actual protocol.
 *
 * @code
 * // One party runs
 * auto config = Discovery::RunServer(42, Party{0, "server_ip", 1234});
 *
 * // Everyone else
 * auto config = Discovery::RunClient("server_ip", 1234, my_id, my_port);
 *
 * // use the config from discovery in the actual protocol
 * auto Network = Network::Create(config);
 * proto::Evaluate(std::move(my_protocol), network);
 * @endcode
 */
struct Discovery {
  /**
   * @brief Run peer discovery as a client.
   * @param server_hostname hostname of the discovery server.
   * @param server_port port of the discovery server.
   * @param my_id ID of this party.
   * @param my_port port of this party.
   * @return a NetworkConfig with peer information.
   */
  static NetworkConfig RunClient(const std::string& server_hostname,
                                 std::size_t server_port,
                                 std::size_t my_id,
                                 std::size_t my_port);

  /**
   * @brief Run peer discovery server.
   * @param number_of_parties the number of peers to discover
   * @param me this party
   * @return a NetworkConfig with peer information.
   */
  static NetworkConfig RunServer(std::size_t number_of_parties,
                                 const Party& me);
};

}  // namespace scl::net

#endif  // SCL_NET_DISCOVERY_DISCOVERY_H
