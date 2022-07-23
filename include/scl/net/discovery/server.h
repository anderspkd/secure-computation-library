/**
 * @file server.h
 *
 * SCL --- Secure Computation Library
 * Copyright (C) 2022 Anders Dalskov
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

#ifndef SCL_NET_DISCOVERY_SERVER_H
#define SCL_NET_DISCOVERY_SERVER_H

#include "scl/net/config.h"
#include "scl/net/network.h"
#include "scl/p/simple.h"

/**
 * @brief The maximum number of parties we allow to be discovered.
 */
#ifndef MAX_DISCOVER_PARTIES
#define MAX_DISCOVER_PARTIES 255
#endif

/**
 * @brief Default port for the discovery server.
 */
#ifndef DEFAULT_DISCOVERY_PORT
#define DEFAULT_DISCOVERY_PORT 9999
#endif

namespace scl {

/**
 * @brief Server for coordinating party discovery.
 */
class DiscoveryServer {
 public:
  /**
   * @brief Create a new discovery coordination server.
   * @param discovery_port the port this server should listen on
   * @param number_of_parties the number of parties to coordinate
   */
  DiscoveryServer(int discovery_port, std::size_t number_of_parties)
      : mPort(discovery_port), mNumberOfParties(number_of_parties) {
    if (number_of_parties > MAX_DISCOVER_PARTIES)
      throw std::invalid_argument("number_of_parties exceeds max");
  };

  /**
   * @brief Create a new discovery coordination server.
   * @param number_of_parties the number of parties to coordinate
   */
  DiscoveryServer(std::size_t number_of_parties)
      : DiscoveryServer(DEFAULT_DISCOVERY_PORT, number_of_parties){};

  /**
   * @brief Run the discovery coordination protocol.
   * @param me ID, port and hostname information for this party
   * @return A network configuration.
   */
  NetworkConfig Run(const Party& me);

  class CollectIdsAndPorts;
  class SendNetworkConfig;

  /**
   * @brief Context object for the discovery server protocol.
   */
  struct Ctx {
    /**
     * @brief Information about the server.
     */
    Party me;
    /**
     * @brief Network for communication.
     */
    Network network;
  };

 private:
  int mPort;
  std::size_t mNumberOfParties;
};

/**
 * @brief First step of discovery: Collects IDs and computation ports.
 *
 * Using connections establish in the previous connection, this step receives
 * from each party its ID and the port it wishes to use for further
 * communication (i.e., the port that will be included in the network config).
 */
class DiscoveryServer::CollectIdsAndPorts
    : scl::ProtocolStep<DiscoveryServer::CollectIdsAndPorts,
                        DiscoveryServer::Ctx> {
 public:
  /**
   * @brief Constructor.
   */
  CollectIdsAndPorts(std::vector<std::string> hostnames)
      : mHostnames(hostnames){};

  /**
   * @brief Run this protocol step.
   */
  DiscoveryServer::SendNetworkConfig Run(DiscoveryServer::Ctx& ctx);

 private:
  std::vector<std::string> mHostnames;
};

/**
 * @brief Final step of discovery: Send the network config object to all
 * parties.
 */
class DiscoveryServer::SendNetworkConfig
    : scl::LastProtocolStep<DiscoveryServer::SendNetworkConfig,
                            DiscoveryServer::Ctx> {
 public:
  /**
   * @brief Constructor.
   */
  SendNetworkConfig(const NetworkConfig& config) : mConfig(config){};

  /**
   * @brief Run the final step of the discovery protocol.
   */
  NetworkConfig Finalize(DiscoveryServer::Ctx& ctx);

 private:
  NetworkConfig mConfig;
};

}  // namespace scl

#endif  // SCL_NET_DISCOVERY_SERVER_H
