/* SCL --- Secure Computation Library
 * Copyright (C) 2024 Anders Dalskov
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
#include <thread>

#include "scl/simulation/config.h"
#include "scl/simulation/context.h"
#include "scl/simulation/env.h"
#include "scl/simulation/event.h"
#include "scl/simulation/mem_channel_buffer.h"

using namespace scl;

namespace {

auto SomeEvent() {
  return std::make_shared<sim::Event>(sim::Event::Type::START,
                                      util::Time::Duration::zero());
}

auto SomeEvent(util::Time::Duration t) {
  return std::make_shared<sim::Event>(sim::Event::Type::START, t);
}

auto DefaultNetworkConfig() {
  return std::make_shared<sim::SimpleNetworkConfig>();
}

}  // namespace

TEST_CASE("Simulation env clock", "[sim]") {
  using namespace std::chrono_literals;

  auto ctx = sim::Context::Create<sim::MemoryBackedChannelBuffer>(
      5,
      DefaultNetworkConfig());
  sim::Clock clock(ctx, 0);

  ctx->AddEvent(0, SomeEvent());
  ctx->UpdateCheckpoint();

  std::this_thread::sleep_for(50ms);

  auto t0 = clock.Read();
  REQUIRE(t0 > 50ms);
  REQUIRE(t0 < 75ms);

  std::this_thread::sleep_for(50ms);
  auto t1 = clock.Read();
  REQUIRE(t1 > 100ms);
  REQUIRE(t1 < 125ms);

  ctx->AddEvent(0, SomeEvent(10 * t1));

  auto t2 = clock.Read();
  REQUIRE(t2 > 1050ms);
  REQUIRE(t2 < 1200ms);
}

TEST_CASE("Simulation env clock checkpoint", "[sim]") {
  using namespace std::chrono_literals;

  auto ctx = sim::Context::Create<sim::MemoryBackedChannelBuffer>(
      5,
      DefaultNetworkConfig());
  sim::Clock clock(ctx, 0);

  ctx->AddEvent(0, SomeEvent(10ms));
  ctx->UpdateCheckpoint();
  clock.Checkpoint("asd");
  REQUIRE(ctx->Trace(0).size() == 2);
  REQUIRE(ctx->Trace(0).back()->EventType() == sim::Event::Type::CHECKPOINT);
  REQUIRE(ctx->Trace(0).back()->Timestamp() >= 10ms);

  sim::CheckpointEvent* e = (sim::CheckpointEvent*)ctx->Trace(0).back().get();
  REQUIRE(e->Id() == "asd");
}

TEST_CASE("Simulation env thread", "[sim]") {
  using namespace std::chrono_literals;
  auto ctx = sim::Context::Create<sim::MemoryBackedChannelBuffer>(
      5,
      DefaultNetworkConfig());

  ctx->UpdateCheckpoint();

  sim::ThreadCtx thread(ctx, 0);
  sim::Clock clock(ctx, 0);

  ctx->AddEvent(0, SomeEvent(util::Time::Duration(1000ms)));
  thread.Sleep(util::Time::Duration(400h));

  auto t0 = clock.Read();
  REQUIRE(t0 > 1000ms + 400h);
  REQUIRE(t0 < 1050ms + 400h);
}
