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

#ifndef SCL_SIMULATION_NETWORK_DESCRIPTION_H
#define SCL_SIMULATION_NETWORK_DESCRIPTION_H

#include <cstddef>
#include <random>
#include <unordered_map>

#include "scl/simulation/channel_id.h"

namespace scl {

struct ChannelDescription final {
  std::size_t bandwith;
  std::size_t latency;
  std::size_t latency_jitter;
  float loss;
};

class NetworkDescription final {
 public:
  struct ChannelParameters {
    std::size_t bandwidth;
    std::size_t latency;
    float loss;
  };

  static NetworkDescription createDefaultLAN(std::size_t size);
  static NetworkDescription createDefaultWAN(std::size_t size);

  std::size_t size() const {
    return m_size;
  }

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

#endif  // SCL_SIMULATION_NETWORK_DESCRIPTION_H
