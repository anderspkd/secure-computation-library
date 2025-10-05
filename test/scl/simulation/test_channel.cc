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

#include <catch2/catch_test_macros.hpp>
#include <memory>

#include "scl/net/packet.h"
#include "scl/simulation/channel.h"
#include "scl/simulation/context.h"
#include "scl/simulation/event.h"
#include "scl/simulation/network_description.h"
#include "scl/simulation/runtime.h"
#include "scl/simulation/transport.h"

using namespace scl;
using namespace std::chrono_literals;

TEST_CASE("SimulatedChannel send", "[sim]") {
  auto nd = NetworkDescription::createDefaultLAN(2);
  auto ctx = details::SimulatorContext::create(nd, {});

  auto tp = std::make_shared<details::Transport>(ctx);
  ChannelId id{0, 1};
  auto channel = details::SimulatedChannel::create(id, ctx.getContext(0), tp);

  details::SimulatorRuntime srt(ctx);

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

TEST_CASE("SimulatedChannel recv", "[sim]") {
  auto nd = NetworkDescription::createDefaultLAN(2);
  auto ctx = details::SimulatorContext::create(nd, {});
  auto tp = std::make_shared<details::Transport>(ctx);
  ChannelId id{0, 1};
  auto channel = details::SimulatedChannel::create(id, ctx.getContext(0), tp);

  details::SimulatorRuntime srt(ctx);

  Packet pkt;
  pkt << 1 << 2 << 3;

  tp->send(Time::Duration::zero(), id.flip(), pkt);

  auto rpkt = srt.run(channel->recv());

  REQUIRE(rpkt.read<int>() == 1);
  REQUIRE(rpkt.read<int>() == 2);
  REQUIRE(rpkt.read<int>() == 3);

  auto ctx0 = ctx.getContext(0);
  REQUIRE(ctx0.lastEvent()->type() == scl::EventType::CHANNEL_RECV);
  REQUIRE(ctx0.lastEvent()->time() > Time::Duration::zero());

  // if the send happened waay in the future, then the receiver's clock will
  // advance at least until that time as recvs are blocking

  tp->send(1000h, id.flip(), pkt);

  rpkt = srt.run(channel->recv());
  REQUIRE(ctx0.lastEvent()->type() == scl::EventType::CHANNEL_RECV);
  REQUIRE(ctx0.lastEvent()->time() > 1000h);
}

TEST_CASE("SimulatedChannel recv timeout") {
  using namespace std::chrono_literals;

  auto nd = NetworkDescription::createDefaultLAN(2);
  auto ctx = details::SimulatorContext::create(nd, {});
  auto tp = std::make_shared<details::Transport>(ctx);
  ChannelId id{0, 1};

  auto channel = details::SimulatedChannel::create(id, ctx.getContext(0), tp);

  details::SimulatorRuntime srt(ctx);

  Packet pkt;
  pkt << 1 << 2 << 3;

  tp->send(100ms, id.flip(), pkt);
  ctx.getContext(1).addEvent<BeginEvent>(100ms, "");

  // should not time out, receiving time should be ~100ms
  auto opt_rpkt = srt.run(channel->recv(200ms));

  REQUIRE(opt_rpkt.has_value());
  auto rpkt = opt_rpkt.value();

  REQUIRE(rpkt.read<int>() == 1);
  REQUIRE(rpkt.read<int>() == 2);
  REQUIRE(rpkt.read<int>() == 3);

  auto ctx0 = ctx.getContext(0);
  REQUIRE(ctx0.lastEvent()->type() == EventType::CHANNEL_RECV);
  REQUIRE(ctx0.lastEvent()->time() > 100ms);

  tp->send(400ms, id.flip(), pkt);
  // have to move the sender ahead of the receiver, otherwise the receiver just
  // gets stuck in an infinite loop (which is correct behavior btw).
  ctx.getContext(1).addEvent<BeginEvent>(500ms, "");

  // should timeout
  opt_rpkt = srt.run(channel->recv(200ms));

  REQUIRE_FALSE(opt_rpkt.has_value());

  REQUIRE(ctx0.lastEvent()->type() == EventType::CHANNEL_RECV_TIMEOUT);
  REQUIRE(ctx0.lastEvent()->time() > 300ms);
}
