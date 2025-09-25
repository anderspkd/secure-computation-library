#include <cmath>
#include <cstddef>
#include <limits>
#include <unordered_map>

#include "scl/simulation/channel_id.h"
#include "scl/simulation/network_description.h"

using namespace scl;

const ChannelDescription DEFAULT_LAN_PARAMS = {.bandwith = 100000000000,
                                               .latency = 100,
                                               .latency_jitter = 0,
                                               .loss = 0.0};

const ChannelDescription DEFAULT_LOCAL_PARAMS = {
    .bandwith = std::numeric_limits<std::size_t>::max(),
    .latency = 0,
    .latency_jitter = 0,
    .loss = 0.0};

NetworkDescription NetworkDescription::createDefaultLAN(std::size_t size) {
  std::unordered_map<ChannelId, ChannelDescription> channels;
  std::unordered_map<ChannelId, JitterSampler> ignored;

  for (std::size_t i = 0; i < size; i++) {
    channels[ChannelId(i, i)] = DEFAULT_LOCAL_PARAMS;
    for (std::size_t j = i + 1; j < size; j++) {
      channels[ChannelId(i, j)] = DEFAULT_LAN_PARAMS;
      channels[ChannelId(j, i)] = DEFAULT_LAN_PARAMS;
    }
  }

  return NetworkDescription{channels, ignored, size};
}

NetworkDescription::ChannelParameters descriptionToParams(
    const ChannelDescription& desc) {
  return NetworkDescription::ChannelParameters{.bandwidth = desc.bandwith,
                                               .latency = desc.latency,
                                               .loss = desc.loss};
}

NetworkDescription::ChannelParameters NetworkDescription::getChannel(
    ChannelId id) {
  const auto desc = m_channels[id];
  auto params = descriptionToParams(desc);
  if (m_samplers.contains(id)) {
    params.latency = std::lround(m_samplers[id]());
  }
  return params;
}
