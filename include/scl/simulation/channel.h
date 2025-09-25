#ifndef SCL_SIMULATION_CHANNEL_H
#define SCL_SIMULATION_CHANNEL_H

#include "scl/net/channel.h"
#include "scl/net/packet.h"
#include "scl/simulation/channel_id.h"
#include "scl/simulation/context.h"
#include "scl/simulation/transport.h"

namespace scl {

class SimulatedChannel final : public Channel {
 public:
  static std::shared_ptr<Channel> create(ChannelId id,
                                         Context ctx,
                                         std::shared_ptr<Transport> transport) {
    return std::make_shared<SimulatedChannel>(id, ctx, transport);
  }

  SimulatedChannel(ChannelId id,
                   Context ctx,
                   std::shared_ptr<Transport> transport)
      : m_id(id), m_ctx(ctx), m_transport(transport) {}

  void close() override;
  Task<void> send(Packet&& packet) override;
  Task<void> send(const Packet& packet) override;
  Task<Packet> recv() override;
  Task<std::optional<Packet>> recv(Time::Duration timeout) override;
  Task<bool> poll() override;

 private:
  ChannelId m_id;
  Context m_ctx;
  std::shared_ptr<Transport> m_transport;
};

}  // namespace scl

#endif  // SCL_SIMULATION_CHANNEL_H
