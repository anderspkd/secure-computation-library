#include <catch2/catch_test_macros.hpp>

#include "scl/net/packet.h"
#include "scl/simulation/channel.h"
#include "scl/simulation/context.h"
#include "scl/simulation/event.h"
#include "scl/simulation/network_description.h"
#include "scl/simulation/runtime.h"
#include "scl/simulation/transport.h"

using namespace scl;

TEST_CASE("SimulatedChannel tests") {
  auto nd = NetworkDescription::createDefaultLAN(2);
  auto ctx = SimulatorContext::create(nd, {});

  auto tp = std::make_shared<Transport>(ctx);
  ChannelId id{0, 1};
  auto channel = SimulatedChannel::create(id, ctx.getContext(0), tp);

  SimulatorRuntime srt(ctx);

  // send something
  Packet pkt;
  pkt << 1 << 2 << 3;

  REQUIRE_FALSE(tp->ready(id.flip()));

  srt.run(channel->send(pkt));

  REQUIRE(tp->ready(id.flip()));

  auto ctx0 = ctx.getContext(0);
  REQUIRE(ctx0.lastEvent()->type() == scl::EventType::CHANNEL_SEND);

  auto [rpkt, delay] = tp->recv(Time::Duration::zero(), id.flip());

  // we only test that the delay added is positive. Validation of the delay
  // added should be done in a more experimental fashion.
  REQUIRE(delay > scl::Time::Duration::zero());

  REQUIRE(rpkt.read<int>() == 1);
  REQUIRE(rpkt.read<int>() == 2);
  REQUIRE(rpkt.read<int>() == 3);
}
