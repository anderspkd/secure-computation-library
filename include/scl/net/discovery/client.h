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

#ifndef SCL_NET_DISCOVERY_CLIENT_H
#define SCL_NET_DISCOVERY_CLIENT_H

#include <memory>
#include <string>

#include "scl/net/config.h"
#include "scl/net/network.h"
#include "scl/protocol/base.h"
#include "scl/protocol/env.h"

namespace scl::net {

/**
 * @brief Protocol definition for a discovery client.
 */
struct DiscoveryClient {
  class SendInfo;
  class RecvConfig;
};

/**
 * @brief Client discovery step where the client sends its ID and port.
 */
class DiscoveryClient::SendInfo : public proto::Protocol {
 public:
  /**
   * @brief Construct a new SendInfo protocol.
   * @param my_id the ID of this party in the output config.
   * @param my_port the port that this party wishes to use in the output config.
   */
  SendInfo(std::size_t my_id, std::size_t my_port)
      : mId(my_id), mPort(my_port){};

  std::unique_ptr<proto::Protocol> Run(
      proto::ProtocolEnvironment& env) override;

 private:
  std::size_t mId;
  std::size_t mPort;
};

/**
 * @brief Client discovery step where the network config is received.
 */
class DiscoveryClient::RecvConfig : public proto::Protocol {
 public:
  /**
   * @brief Construct a new RecvConfig protocol.
   * @param my_id the ID of this party in the output config.
   */
  RecvConfig(std::size_t my_id) : mId(my_id){};

  std::unique_ptr<proto::Protocol> Run(
      proto::ProtocolEnvironment& env) override;

  std::any Output() const override {
    return mConfig;
  }

 private:
  std::size_t mId;

  NetworkConfig mConfig;
};

}  // namespace scl::net

#endif  // SCL_NET_DISCOVERY_CLIENT_H
