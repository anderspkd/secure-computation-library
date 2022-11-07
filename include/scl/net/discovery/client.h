/**
 * @file client.h
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

#ifndef SCL_NET_DISCOVERY_CLIENT_H
#define SCL_NET_DISCOVERY_CLIENT_H

#include <memory>
#include <string>

#include "scl/net/config.h"
#include "scl/net/discovery/server.h"
#include "scl/net/network.h"
#include "scl/p/simple.h"

namespace scl {

/**
 * @brief Client that receives a network config from a remote server.
 */
class DiscoveryClient {
 public:
  /**
   * @brief Create a new client in a discovery protocol.
   * @param discovery_port the port of the discovery server
   * @param discovery_hostname the hostname of the discovery server
   */
  DiscoveryClient(const std::string& discovery_hostname, int discovery_port)
      : mHostname(discovery_hostname), mPort(discovery_port){};

  /**
   * @brief Create a new client in a discovery protocol.
   * @param discovery_hostname the hostname of the discovery server
   */
  DiscoveryClient(const std::string& discovery_hostname)
      : DiscoveryClient(discovery_hostname, DEFAULT_DISCOVERY_PORT){};

  /**
   * @brief Run the discovery protocol.
   * @param id the ID of this party
   * @param port the port of this party
   * @return A network configuration.
   */
  NetworkConfig Run(int id, int port) const;

  class SendIdAndPort;
  class ReceiveNetworkConfig;

 private:
  std::string mHostname;
  int mPort;
};

/**
 * @brief Send this party's ID and port to the discovery server.
 */
class DiscoveryClient::SendIdAndPort
    : scl::ProtocolStep<DiscoveryClient::SendIdAndPort,
                        std::shared_ptr<Channel>> {
 public:
  /**
   * @brief Constructor.
   */
  SendIdAndPort(int id, int port) : mId(id), mPort(port){};

  /**
   * @brief Run this protocol step.
   */
  DiscoveryClient::ReceiveNetworkConfig Run(
      const std::shared_ptr<Channel>& ctx) const;

 private:
  int mId;
  int mPort;
};

/**
 * @brief Receive a network configuration from a discovery server.
 */
class DiscoveryClient::ReceiveNetworkConfig
    : scl::LastProtocolStep<DiscoveryClient::ReceiveNetworkConfig,
                            std::shared_ptr<Channel>> {
 public:
  /**
   * @brief Constructor.
   */
  ReceiveNetworkConfig(int id) : mId(id){};

  /**
   * @brief Finalize the discovery protocol.
   */
  NetworkConfig Finalize(const std::shared_ptr<Channel>& ctx) const;

 private:
  int mId;
};

}  // namespace scl

#endif  // SCL_NET_DISCOVERY_CLIENT_H
