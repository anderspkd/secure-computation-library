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

#pragma once

#include <cstddef>
#include <random>
#include <unordered_map>
#include <variant>

#include "scl/simulation/channel_id.h"

namespace scl {

/**
 * @brief Provider for a communication channels paramters.
 * @ingroup eval-sim
 *
 * A ChannelParams object describes the characteristics of a particular
 * communication channel in terms of its bandwidth and latency.
 *
 * A communication channel is either "deterministic" or "probabilistic", with
 * respect to its parameters. Concretely, either the channel's parameters are
 * fixed or they are sampled from some distribution.
 *
 * Note that the packet loss of a channel is a (by its nature) probabilistic
 * parameter regardless of the type of the other parameters.
 *
 * The units used is as follows:
 * - bits per second for bandwidth.
 * - microseconds for latency.
 * - percentage, as a value between 0 and 1 for packet loss.
 */
class ChannelParams final {
 public:
  /**
   * @brief Create a deterministic channel description.
   */
  static ChannelParams createDet(std::size_t bandwidth,
                                 std::size_t latency,
                                 float packet_loss) {
    return ChannelParams(DetParams{bandwidth, latency}, packet_loss);
  }

  /**
   * @brief Create a probabilistic channel description.
   */
  static ChannelParams createProb(std::normal_distribution<> bandwidth,
                                  std::normal_distribution<> latency,
                                  float packet_loss) {
    std::random_device rd{};
    std::mt19937 rg{rd()};
    return ChannelParams(PropParams{rg, bandwidth, latency}, packet_loss);
  }

  ChannelParams() {}

  /**
   * @brief Bandwidth of the channel.
   */
  std::size_t bandwidth() const;

  /**
   * @brief Latency of the channel.
   */
  std::size_t latency() const;

  /**
   * @brief The packet loss percentage of this channel.
   */
  float packetLoss() const {
    return m_packet_loss;
  }

 private:
  struct DetParams final {
    std::size_t bandwidth;
    std::size_t latency;
  };

  struct PropParams final {
    std::mt19937 rg;

    std::normal_distribution<> bandwidth;
    std::normal_distribution<> latency;
  };

  ChannelParams(DetParams params, float packet_loss)
      : m_params{params}, m_packet_loss{packet_loss} {}
  ChannelParams(PropParams params, float packet_loss)
      : m_params{params}, m_packet_loss{packet_loss} {}

  bool deterministicChannel() const {
    return m_params.index() == 0;
  }

  mutable std::variant<DetParams, PropParams> m_params;
  float m_packet_loss;
};

/**
 * @brief A parameter collection for a network.
 * @ingroup eval-sim
 *
 * NetworkParams can be viewed as a collection of ChannelParams (as that is what
 * it ultimately is).
 */
class NetworkParams final {
 public:
  /**
   * @brief Create a set of network parameters with some sane defaults.
   *
   * This creates a new set of network parameters supporting a specified number
   * of parties. The provided \p bandwidth and \p latency arguments determine
   * the bandwidth and latency for all channels that are not "loopback"
   * channels.
   *
   * The default values provide a sane "WAN"-like default of 1Mbps bandwidth,
   * 50ms latency and 0.1% packet loss.
   */
  static NetworkParams create(std::size_t number_of_parties,
                              std::size_t bandwidth = 1000000,
                              std::size_t latency = 50000,
                              float packet_loss = 0.01);

  /**
   * @brief Read network parameters from a file.
   */
  static NetworkParams fromFile(const std::string& filename);

  /**
   * @brief Construct network parameters from a set of channel parameters.
   */
  NetworkParams(const std::unordered_map<ChannelId, ChannelParams>& channels)
      : m_channels(channels) {}

  /**
   * @brief Get the parameters of a channel.
   */
  ChannelParams channel(ChannelId id) const {
    return m_channels.at(id);
  }

 private:
  std::unordered_map<ChannelId, ChannelParams> m_channels;
};

}  // namespace scl
