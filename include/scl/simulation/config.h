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

#ifndef SCL_SIMULATION_CONFIG_H
#define SCL_SIMULATION_CONFIG_H

#include <any>
#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>

#include "scl/simulation/channel_id.h"

namespace scl::sim {

/**
 * @brief Configuration for a channel between two parties.
 */
class ChannelConfig {
 public:
  /**
   * @brief Builder used to create network configs.
   */
  class Builder;

  /**
   * @brief Indicates which type of network the channel is emulating.
   */
  enum class NetworkType {
    /**
     * @brief The channel is a TCP channel.
     */
    TCP,

    /**
     * @brief The channel is a special channel where communication is instant.
     */
    INSTANT,
  };

  /**
   * @brief Default network type is TCP.
   */
  constexpr static NetworkType DEFAULT_NETWORK_TYPE = NetworkType::TCP;

  /**
   * @brief Default bandwidth of the simulated network, in bits/s.
   */
  constexpr static std::size_t DEFAULT_BANDWIDTH = 1000000;

  /**
   * @brief Default RTT of the simulated network in ms.
   */
  constexpr static std::size_t DEFAULT_RTT = 100;

  /**
   * @brief Default MSS in bytes.
   */
  constexpr static std::size_t DEFAULT_MSS = 1460;

  /**
   * @brief Default package loss in percentage.
   */
  constexpr static double DEFAULT_PACKAGE_LOSS = 0;

  /**
   * @brief Default TCP window size in bytes.
   */
  constexpr static std::size_t DEFAULT_WINDOW_SIZE = 65536;

  /**
   * @brief Create a simulation config with default values.
   */
  static ChannelConfig Default();

  /**
   * @brief Create a simulation config for a loopback connection.
   */
  static ChannelConfig Loopback();

  /**
   * @brief The network type of the channel.
   */
  NetworkType Type() const {
    return m_type;
  }

  /**
   * @brief Bandwidth in Bits/s.
   */
  std::size_t Bandwidth() const {
    return m_bandwidth;
  }

  /**
   * @brief RTT in ms.
   */
  std::size_t RTT() const {
    return m_rtt;
  }

  /**
   * @brief MSS in bytes.
   */
  std::size_t MSS() const {
    return m_MSS;
  };

  /**
   * @brief Package loss in percentage.
   */
  double PackageLoss() const {
    return m_package_loss;
  }

  /**
   * @brief TCP window size.
   */
  std::size_t WindowSize() const {
    return m_window_size;
  };

 private:
  ChannelConfig(NetworkType type,
                std::size_t bandwidth,
                std::size_t rtt,
                std::size_t MSS,
                double package_loss,
                std::size_t window_size)
      : m_type(type),
        m_bandwidth(bandwidth),
        m_rtt(rtt),
        m_MSS(MSS),
        m_package_loss(package_loss),
        m_window_size(window_size){};

  NetworkType m_type;
  std::size_t m_bandwidth;
  std::size_t m_rtt;
  std::size_t m_MSS;
  double m_package_loss;
  std::size_t m_window_size;
};

/**
 * @brief Pretty print the simulation config.
 */
std::ostream& operator<<(std::ostream& os, const ChannelConfig& config);

/**
 * @brief Builder used to create network configs.
 */
class ChannelConfig::Builder {
 public:
  /**
   * @brief Create an empty simulation config builder.
   */
  Builder(){};

  /**
   * @brief Build the simulation config.
   */
  ChannelConfig Build() const {
    Validate();
    return ChannelConfig{
        m_type.value_or(ChannelConfig::DEFAULT_NETWORK_TYPE),
        m_bandwidth.value_or(ChannelConfig::DEFAULT_BANDWIDTH),
        m_rtt.value_or(ChannelConfig::DEFAULT_RTT),
        m_MSS.value_or(ChannelConfig::DEFAULT_MSS),
        m_package_loss.value_or(ChannelConfig::DEFAULT_PACKAGE_LOSS),
        m_window_size.value_or(ChannelConfig::DEFAULT_WINDOW_SIZE)};
  };

  /**
   * @brief Set the network type of this channel.
   * @param type the network type.
   * @return the builder.
   */
  Builder& Type(NetworkType type) {
    m_type = type;
    return *this;
  }

  /**
   * @brief Set network bandwidth to use for the simulation.
   * @param bandwidth bandwidth in bits/s.
   * @return the builder.
   */
  Builder& Bandwidth(std::size_t bandwidth) {
    m_bandwidth = bandwidth;
    return *this;
  }

  /**
   * @brief Set the network RTT to use for the simulation.
   * @param value latency in ms
   * @return the builder.
   */
  Builder& RTT(std::size_t value) {
    m_rtt = value;
    return *this;
  }

  /**
   * @brief Set the Message Segment Size (MSS) used in the simulation.
   * @param MSS the MSS
   * @return the builder.
   */
  Builder& MSS(std::size_t MSS) {
    m_MSS = MSS;
    return *this;
  }

  /**
   * @brief Set the package loss.
   * @param percentage the percent of packages being lost
   * @return the builder.
   */
  Builder& PackageLoss(double percentage) {
    m_package_loss = percentage;
    return *this;
  }

  /**
   * @brief Set the TCP window size.
   * @param window_size of the TCP window in bytes
   * @return the builder.
   */
  Builder& WindowSize(std::size_t window_size) {
    m_window_size = window_size;
    return *this;
  }

 private:
  std::optional<NetworkType> m_type;
  std::optional<std::size_t> m_bandwidth;
  std::optional<std::size_t> m_rtt;
  std::optional<std::size_t> m_MSS;
  std::optional<double> m_package_loss;
  std::optional<std::size_t> m_window_size;

  // Validate the config settings before creating the actual SimulationConfig.
  void Validate() const;
};

/**
 * @brief Interface describing the network wide configuration.
 */
struct NetworkConfig {
  /**
   * @brief Destructor.
   */
  virtual ~NetworkConfig() {}

  /**
   * @brief Returns the configuration of a particular channel.
   */
  virtual ChannelConfig Get(ChannelId channel_id) = 0;
};

/**
 * @brief Network configuration for a simple network.
 *
 * SimpleNetworkConfig describes a network where everyone is connected on a
 * channel configured according to ChannelConfig::Default. The only exception
 * being channels that are self-connecting (i.e., from a party to itself). These
 * channels are configured according to ChannelConfig::Loopback.
 */
struct SimpleNetworkConfig final : public NetworkConfig {
  ChannelConfig Get(ChannelId channel_id) override {
    static auto config = ChannelConfig::Default();
    static auto lo = ChannelConfig::Loopback();

    return (channel_id.local == channel_id.remote) ? lo : config;
  }
};

}  // namespace scl::sim

#endif  // SCL_SIMULATION_CONFIG_H
