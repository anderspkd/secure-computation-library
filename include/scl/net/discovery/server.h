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

#ifndef SCL_NET_DISCOVERY_SERVER_H
#define SCL_NET_DISCOVERY_SERVER_H

#include <memory>
#include <string>
#include <vector>

#include "scl/net/config.h"
#include "scl/net/network.h"
#include "scl/protocol/base.h"
#include "scl/protocol/env.h"

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

namespace scl::net {

/**
 * @brief Protocol definition for the discovery server.
 */
struct DiscoveryServer {
  class RecvInfo;
  class SendConfig;
};

/**
 * @brief Discovery server protocol step where the server receives IDs and ports
 * from peers.
 */
class DiscoveryServer::RecvInfo : public proto::Protocol {
 public:
  /**
   * @brief Constructo a new RecvInfo protocol.
   * @param me the server's information in the output config.
   * @param hostnames hostnames of all the peers.
   */
  RecvInfo(const Party& me, const std::vector<std::string>& hostnames)
      : mMe(me), mHostnames(hostnames){};

  std::unique_ptr<proto::Protocol> Run(
      proto::ProtocolEnvironment& env) override;

 private:
  Party mMe;
  std::vector<std::string> mHostnames;
};

/**
 * @brief Discovery server protocol step where the server sends the network
 * config to all the other parties.
 */
class DiscoveryServer::SendConfig : public proto::Protocol {
 public:
  /**
   * @brief Construct a new SendConfig protocol.
   * @param config the config to send.
   */
  SendConfig(const NetworkConfig& config) : mConfig(config){};

  std::unique_ptr<proto::Protocol> Run(
      proto::ProtocolEnvironment& env) override;

  std::any Output() const override {
    return mConfig;
  }

 private:
  NetworkConfig mConfig;
};

}  // namespace scl::net

#endif  // SCL_NET_DISCOVERY_SERVER_H
