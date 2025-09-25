#ifndef SCL_SIMULATION_TRANSPORT_H
#define SCL_SIMULATION_TRANSPORT_H

#include <queue>
#include <unordered_map>

#include "scl/net/packet.h"
#include "scl/simulation/channel_id.h"
#include "scl/simulation/context.h"
#include "scl/time.h"

namespace scl {

class Transport {
 public:
  Transport(SimulatorContext& sim_ctx) : m_sim_ctx(sim_ctx) {}

  void send(Time::Duration ts, ChannelId id, const Packet& pkt);
  void send(Time::Duration ts, ChannelId id, Packet&& pkt);

  bool ready(ChannelId id) const;

  std::pair<Packet, Time::Duration> recv(Time::Duration ts, ChannelId id);

  enum class PollResult { NA, DATA, NO_DATA };

  PollResult poll(Time::Duration ts, ChannelId id) const;

 private:
  SimulatorContext& m_sim_ctx;
  std::unordered_map<ChannelId, std::queue<std::pair<Packet, Time::Duration>>>
      m_pqs;
};

}  // namespace scl

#endif  // SCL_SIMULATION_TRANSPORT_H
