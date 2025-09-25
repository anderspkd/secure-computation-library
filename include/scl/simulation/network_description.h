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
