#ifndef SCL_SIMULATION_CHANNEL_ID_H
#define SCL_SIMULATION_CHANNEL_ID_H

#include <cstddef>
#include <functional>

namespace scl {

struct ChannelId final {
  std::size_t local;
  std::size_t remote;

  auto operator<=>(const ChannelId& other) const = default;

  ChannelId flip() const {
    return ChannelId{remote, local};
  }
};

}  // namespace scl

template <>
struct std::hash<scl::ChannelId> {
  std::size_t operator()(const scl::ChannelId& channel_id) const {
    return channel_id.local ^ (channel_id.remote << 32);
  }
};

#endif  // SCL_SIMULATION_CHANNEL_ID_H
