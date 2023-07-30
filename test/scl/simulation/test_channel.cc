/* SCL --- Secure Computation Library
 * Copyright (C) 2023 Anders Dalskov
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

#include <catch2/catch.hpp>
#include <memory>

#include "scl/simulation/channel.h"
#include "scl/simulation/config.h"
#include "scl/simulation/context.h"
#include "scl/simulation/event.h"
#include "scl/simulation/mem_channel_buffer.h"
#include "scl/simulation/simulator.h"

using namespace scl;

namespace {

struct InstantNetworkConfig final : sim::NetworkConfig {
  sim::ChannelConfig Get(sim::ChannelId channel_id) override {
    (void)channel_id;
    return sim::ChannelConfig::Loopback();
  }
};

auto StartEvent(util::Time::Duration ts) {
  return std::make_shared<sim::Event>(sim::Event::Type::START, ts);
}

auto StopEvent(util::Time::Duration ts) {
  return std::make_shared<sim::Event>(sim::Event::Type::STOP, ts);
}

}  // namespace

TEST_CASE("Channel recv packet blocking", "[sim]") {
  auto cfg = std::make_shared<InstantNetworkConfig>();
  auto ctx = sim::Context::Create<sim::MemoryBackedChannelBuffer>(2, cfg);
  auto chl0 = sim::Channel({0, 1}, ctx);
  auto chl1 = sim::Channel({1, 0}, ctx);

  net::Packet p;
  p << 123;
  ctx->AddEvent(0, StartEvent(util::Time::Duration::zero()));
  chl0.Send(p);
  const auto t0 = ctx->Trace(0);
  REQUIRE(t0.size() == 2);
  REQUIRE(t0[0]->EventType() == sim::Event::Type::START);
  REQUIRE(t0[1]->EventType() == sim::Event::Type::PACKET_SEND);

  ctx->AddEvent(1, StartEvent(util::Time::Duration::zero()));
  chl1.Recv();

  const auto t1 = ctx->Trace(1);
  REQUIRE(t1.size() == 2);
  REQUIRE(t1[0]->EventType() == sim::Event::Type::START);
  REQUIRE(t1[1]->EventType() == sim::Event::Type::PACKET_RECV);
}

TEST_CASE("Channel recv packet non-blocking", "[sim]") {
  auto cfg = std::make_shared<InstantNetworkConfig>();
  auto ctx = sim::Context::Create<sim::MemoryBackedChannelBuffer>(2, cfg);
  auto chl0 = sim::Channel({0, 1}, ctx);
  auto chl1 = sim::Channel({1, 0}, ctx);

  net::Packet p;
  p << 123;
  ctx->AddEvent(0, StartEvent(util::Time::Duration(1000)));
  chl0.Send(p);

  ctx->AddEvent(1, StartEvent(util::Time::Duration::zero()));
  auto pkt = chl1.Recv(false);

  REQUIRE_FALSE(pkt.has_value());
  auto t0 = ctx->Trace(1);
  REQUIRE(t0.size() == 2);
  REQUIRE(t0[0]->EventType() == sim::Event::Type::START);
  REQUIRE(t0[1]->EventType() == sim::Event::Type::PACKET_RECV);

  ctx->AddEvent(1, StartEvent(ctx->LatestTimestamp(0)));
  auto pkt0 = chl1.Recv(false);

  REQUIRE(pkt0.has_value());
  t0 = ctx->Trace(1);
  REQUIRE(t0.size() == 4);
  REQUIRE(t0[2]->EventType() == sim::Event::Type::START);
  REQUIRE(t0[3]->EventType() == sim::Event::Type::PACKET_RECV);
}

TEST_CASE("Channel recv chunked", "[sim]") {
  auto cfg = std::make_shared<InstantNetworkConfig>();
  auto ctx = sim::Context::Create<sim::MemoryBackedChannelBuffer>(2, cfg);
  auto chl0 = sim::Channel({0, 1}, ctx);
  auto chl1 = sim::Channel({1, 0}, ctx);

  unsigned char data[] = {1, 2, 3, 4};
  ctx->AddEvent(0, StartEvent(util::Time::Duration::zero()));
  chl0.Send(data, 4);

  ctx->AddEvent(1, StartEvent(util::Time::Duration::zero()));
  unsigned char recv[4] = {0};

  REQUIRE(ctx->HasWrite({0, 1}));
  REQUIRE(ctx->NextWrite({0, 1}).amount == 4);
  chl1.Recv(recv, 2);

  REQUIRE(ctx->NextWrite({0, 1}).amount == 2);
  chl1.Recv(recv + 2, 2);
  REQUIRE_FALSE(ctx->HasWrite({0, 1}));

  REQUIRE(data[0] == recv[0]);
  REQUIRE(data[1] == recv[1]);
  REQUIRE(data[2] == recv[2]);
  REQUIRE(data[3] == recv[3]);
}

TEST_CASE("Channel HasData no data, but not far ahead", "[sim]") {
  auto cfg = std::make_shared<InstantNetworkConfig>();
  auto ctx = sim::Context::Create<sim::MemoryBackedChannelBuffer>(2, cfg);

  sim::Channel p0({0, 1}, ctx);
  sim::Channel p1({1, 0}, ctx);

  // P1 at time 100000, P0 at time 0. So we can say for sure that P1 does not
  // have data for P0.

  ctx->AddEvent(1, StartEvent(util::Time::Duration(100000)));
  ctx->AddEvent(0, StartEvent(util::Time::Duration::zero()));

  ctx->UpdateCheckpoint();
  auto hd = p0.HasData();
  REQUIRE_FALSE(hd);
}

TEST_CASE("Channel HasData no data, other party terminated", "[sim]") {
  auto cfg = std::make_shared<InstantNetworkConfig>();
  auto ctx = sim::Context::Create<sim::MemoryBackedChannelBuffer>(2, cfg);

  sim::Channel p0({0, 1}, ctx);
  sim::Channel p1({1, 0}, ctx);

  ctx->AddEvent(1, StopEvent(util::Time::Duration::zero()));
  ctx->AddEvent(0, StartEvent(util::Time::Duration::zero()));

  ctx->UpdateCheckpoint();
  auto hd = p0.HasData();
  REQUIRE_FALSE(hd);
}

TEST_CASE("Channel HasData no data, fails", "[sim]") {
  auto cfg = std::make_shared<InstantNetworkConfig>();
  auto ctx = sim::Context::Create<sim::MemoryBackedChannelBuffer>(3, cfg);

  sim::Channel p0({0, 1}, ctx);
  sim::Channel p1({1, 0}, ctx);

  // P1 at time 100000, P0 at time 0. So we can say for sure that P1 does not
  // have data for P0.

  ctx->AddEvent(1, StartEvent(util::Time::Duration::zero()));
  ctx->AddEvent(0, StartEvent(util::Time::Duration::zero()));

  ctx->UpdateCheckpoint();
  REQUIRE_THROWS_MATCHES(p0.HasData(),
                         sim::SimulationFailure,
                         Catch::Matchers::Message("no data, and we're ahead"));
  auto next = ctx->NextToRun(0);
  REQUIRE(next.value_or(-1) == 1);
}

TEST_CASE("Channel HasData other party not started", "[sim]") {
  auto cfg = std::make_shared<InstantNetworkConfig>();
  auto ctx = sim::Context::Create<sim::MemoryBackedChannelBuffer>(3, cfg);

  sim::Channel p0({0, 1}, ctx);
  sim::Channel p1({1, 0}, ctx);

  ctx->AddEvent(0, StartEvent(util::Time::Duration::zero()));

  ctx->UpdateCheckpoint();
  REQUIRE_THROWS_MATCHES(
      p0.HasData(),
      sim::SimulationFailure,
      Catch::Matchers::Message("other party hasnt started yet"));
  auto next = ctx->NextToRun(0);
  REQUIRE(next.value_or(-1) == 1);
}
