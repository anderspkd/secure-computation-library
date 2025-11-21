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

class ChannelDesc final {
 public:
  ChannelDesc createDet(std::size_t bandwidth, std::size_t latency) {
    return ChannelDesc(DetParams{bandwidth, latency});
  }
  ChannelDesc createProp(std::normal_distribution<> bandwidth,
                         std::normal_distribution<> latency) {
    std::random_device rd{};
    std::mt19937 rg{rd()};
    return ChannelDesc(PropParams{rg, bandwidth, latency});
  }

  std::size_t bandwidth();
  std::size_t latency();

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

  ChannelDesc(DetParams params) : m_params(params) {}
  ChannelDesc(PropParams params) : m_params(params) {}

  std::variant<DetParams, PropParams> m_params;
};

// TODO: Create a simplier, and more extensible network description. Potentially
// shorten the name as well.
//
// The new object (interface shown below) should provide the "raw" link
// bandwidth and latency on a given channel. I guess it is possible to
// dynamically adjust the bandwidth of a channel depending on usage, but I think
// that task is better done elsewhere (e.g., in the Transport).
//
// NetworkDesc:
//   std::size_t latency(ChannelId cid);
//   std::size_t bandwidth(ChannelId cid);
//
// ChannelDesc:
//   virtual std::size_t latency();
//   virtual std::size_t bandwidth();
//
// NetworkDesc is internally composed of a list of ChannelDesc. The definition
// of ChannelDesc allows the user to either provide some constant value, or to
// sample the value from a distribution.

/**
 * @brief Describes the characterists of a channel.
 */
struct ChannelDescription final {
  /**
   * @brief The channel bandwidth, in bits/s.
   */
  std::size_t bandwith;

  /**
   * @brief The channel latency, in microseconds.
   */
  std::size_t latency;

  /**
   * @brief The latency variance, in microseconds.
   */
  std::size_t latency_jitter;

  /**
   * @brief The channel package loss, a value in [0, 1).
   */
  float loss;
};

/**
 * @brief Describes a network in terms of its channels.
 */
class NetworkDescription final {
 public:
  /**
   * @brief The parameters of a channel at a given point in time.
   */
  struct ChannelParameters {
    /**
     * @brief The current channel bandwidth.
     */
    std::size_t bandwidth;

    /**
     * @brief The current channel latency.
     */
    std::size_t latency;

    /**
     * @brief The current channel packege loss.
     */
    float loss;
  };

  /**
   * @brief Create a default LAN network.
   */
  static NetworkDescription createDefaultLAN(std::size_t size);

  /**
   * @brief Create a default WAN network.
   */
  static NetworkDescription createDefaultWAN(std::size_t size);

  /**
   * @brief The size of the network.
   */
  std::size_t size() const {
    return m_size;
  }

  /**
   * @brief Get the parameters of a particular channel.
   */
  ChannelParameters getChannel(ChannelId id);

 private:
  struct JitterSampler {
    std::mt19937 rg;
    std::normal_distribution<> dist;

    std::size_t operator()() {
      return std::lround(dist(rg));
    }
  };

  std::unordered_map<ChannelId, ChannelDescription> m_channels;
  std::unordered_map<ChannelId, JitterSampler> m_samplers;

  std::size_t m_size;

  NetworkDescription(
      const std::unordered_map<ChannelId, ChannelDescription>& channels,
      const std::unordered_map<ChannelId, JitterSampler>& samplers,
      std::size_t size)
      : m_channels(channels), m_samplers(samplers), m_size(size) {}
};

}  // namespace scl
