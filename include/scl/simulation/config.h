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
 * @brief Configuration for the simulated network.
 */
class SimulatedNetworkConfig {
 public:
  /**
   * @brief Builder used to create network configs.
   */
  class Builder;

  /**
   * @brief Default bandwidth of the simulated network, in bits/s.
   */
  constexpr static std::size_t kDefaultBandwidth = 1000000;

  /**
   * @brief Default RTT of the simulated network in ms.
   */
  constexpr static std::size_t kDefaultRTT = 100;

  /**
   * @brief Default MSS in bytes.
   */
  constexpr static std::size_t kDefaultMSS = 1460;

  /**
   * @brief Default package loss in percentage.
   */
  constexpr static double kDefaultPackageLoss = 0;

  /**
   * @brief Default TCP window size in bytes.
   */
  constexpr static std::size_t kDefaultWindowSize = 65536;

  /**
   * @brief Create a simulation config with default values.
   */
  static SimulatedNetworkConfig Default();

  /**
   * @brief Create a simulation config for a loopback connection.
   */
  static SimulatedNetworkConfig Loopback();

  /**
   * @brief Bandwidth in Bits/s.
   */
  std::size_t Bandwidth() const {
    return mBandwidth;
  }

  /**
   * @brief RTT in ms.
   */
  std::size_t RTT() const {
    return mRTT;
  }

  /**
   * @brief MSS in bytes.
   */
  std::size_t MSS() const {
    return mMSS;
  };

  /**
   * @brief Package loss in percentage.
   */
  double PackageLoss() const {
    return mPackageLoss;
  }

  /**
   * @brief TCP window size.
   */
  std::size_t WindowSize() const {
    return mWindowSize;
  };

 private:
  SimulatedNetworkConfig(std::size_t bandwidth,
                         std::size_t rtt,
                         std::size_t MSS,
                         double package_loss,
                         std::size_t window_size)
      : mBandwidth(bandwidth),
        mRTT(rtt),
        mMSS(MSS),
        mPackageLoss(package_loss),
        mWindowSize(window_size){};

  std::size_t mBandwidth;
  std::size_t mRTT;
  std::size_t mMSS;
  double mPackageLoss;
  std::size_t mWindowSize;
};

/**
 * @brief Pretty print the simulation config.
 */
std::ostream& operator<<(std::ostream& os,
                         const SimulatedNetworkConfig& config);

/**
 * @brief Builder used to create network configs.
 */
class SimulatedNetworkConfig::Builder {
 public:
  /**
   * @brief Create an empty simulation config builder.
   */
  Builder(){};

  /**
   * @brief Build the simulation config.
   */
  SimulatedNetworkConfig Build() const {
    Validate();
    return SimulatedNetworkConfig{
        mBandwidth.value_or(SimulatedNetworkConfig::kDefaultBandwidth),
        mRTT.value_or(SimulatedNetworkConfig::kDefaultRTT),
        mMSS.value_or(SimulatedNetworkConfig::kDefaultMSS),
        mPackageLoss.value_or(SimulatedNetworkConfig::kDefaultPackageLoss),
        mWindowSize.value_or(SimulatedNetworkConfig::kDefaultWindowSize)};
  };

  /**
   * @brief Set network bandwidth to use for the simulation.
   * @param bandwidth bandwidth in bits/s.
   * @return the builder.
   */
  Builder& Bandwidth(std::size_t bandwidth) {
    mBandwidth = bandwidth;
    return *this;
  }

  /**
   * @brief Set the network RTT to use for the simulation.
   * @param value latency in ms
   * @return the builder.
   */
  Builder& RTT(std::size_t value) {
    mRTT = value;
    return *this;
  }

  /**
   * @brief Set the Message Segment Size (MSS) used in the simulation.
   * @param MSS the MSS
   * @return the builder.
   */
  Builder& MSS(std::size_t MSS) {
    mMSS = MSS;
    return *this;
  }

  /**
   * @brief Set the package loss.
   * @param percentage the percent of packages being lost
   * @return the builder.
   */
  Builder& PackageLoss(double percentage) {
    mPackageLoss = percentage;
    return *this;
  }

  /**
   * @brief Set the TCP window size.
   * @param window_size of the TCP window in bytes
   * @return the builder.
   */
  Builder& WindowSize(std::size_t window_size) {
    mWindowSize = window_size;
    return *this;
  }

 private:
  std::optional<std::size_t> mBandwidth;
  std::optional<std::size_t> mRTT;
  std::optional<std::size_t> mMSS;
  std::optional<double> mPackageLoss;
  std::optional<std::size_t> mWindowSize;

  // Validate the config settings before creating the actual SimulationConfig.
  void Validate() const;
};

/**
 * @brief Creator object for simulation network configs.
 *
 * The creator should be a function which receives a channel identifier and
 * returns network config for that channel.
 */
using SimulatedNetworkConfigCreator =
    std::function<SimulatedNetworkConfig(ChannelId)>;

/**
 * @brief Default config creator implementation.
 *
 * This implementation returns a default config for all channels;
 */
struct DefaultConfigCreator {
  /**
   * @brief Return a config based on a ChannelId.
   * @param channel_id the ChannelId
   *
   * If the channel_id specifies a channel between two different peers, then
   * sim::SimulatedNetworkConfig::Default() is returned, otherwise
   * sim::SimulatedNetworkConfig::Loopback() is returned.
   */
  SimulatedNetworkConfig operator()(ChannelId channel_id) {
    static auto config = SimulatedNetworkConfig::Default();
    static auto lo = SimulatedNetworkConfig::Loopback();

    return (channel_id.local == channel_id.remote) ? lo : config;
  }
};

}  // namespace scl::sim

#endif  // SCL_SIMULATION_CONFIG_H
