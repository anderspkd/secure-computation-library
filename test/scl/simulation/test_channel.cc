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
#include <memory>

#include "scl/coro/runtime.h"
#include "scl/net/loopback.h"
#include "scl/net/packet.h"
#include "scl/simulation/channel.h"
#include "scl/simulation/config.h"
#include "scl/simulation/context.h"
#include "scl/simulation/event.h"
#include "scl/simulation/runtime.h"

using namespace scl;
using namespace std::chrono_literals;

namespace {

std::array<sim::details::SimulatedChannel, 2> createChannels(
    sim::details::GlobalContext& gctx) {
  auto transport = std::make_shared<sim::details::Transport>();
  sim::details::SimulatedChannel channel01({0, 1}, gctx.view(0), transport);
  sim::details::SimulatedChannel channel10({1, 0}, gctx.view(1), transport);
  return {channel01, channel10};
}

std::size_t getChannelDataEventAmount(std::shared_ptr<sim::Event> event_ptr) {
  return std::dynamic_pointer_cast<sim::ChannelDataEvent>(event_ptr)->amount;
}

}  // namespace

TEST_CASE("SimulatedChannel send/recv", "[sim]") {
  auto gctx = sim::details::GlobalContext::create(
      2,
      std::make_unique<sim::SimpleNetworkConfig>(),
      {});
  auto channels = createChannels(gctx);

  gctx.view(0).recordEvent(sim::Event::start());
  gctx.view(1).recordEvent(sim::Event::start());

  auto rt = sim::details::SimulatorRuntime(gctx);

  net::Packet p;
  p << 1 << 2 << 3;

  const std::size_t expected_size =
      sizeof(net::Packet::SizeType) + 3 * sizeof(int);

  gctx.view(0).startClock();
  rt.run(channels[0].send(std::move(p)));
  REQUIRE(gctx.traces[0].size() == 2);
  REQUIRE(gctx.traces[0].back()->type == sim::EventType::SEND);
  REQUIRE(getChannelDataEventAmount(gctx.traces[0].back()) == expected_size);

  REQUIRE(gctx.sends[{0, 1}].size() == 1);
  auto send_ts = gctx.sends[{0, 1}].front();
  REQUIRE(gctx.traces[0].back()->timestamp == send_ts);

  gctx.view(1).startClock();
  auto pr = rt.run(channels[1].recv());
  REQUIRE(pr.read<int>() == 1);
  REQUIRE(pr.read<int>() == 2);
  REQUIRE(pr.read<int>() == 3);

  REQUIRE(gctx.traces[1].size() == 2);
  REQUIRE(gctx.traces[1].back()->type == sim::EventType::RECV);
  REQUIRE(getChannelDataEventAmount(gctx.traces[1].back()) == expected_size);

  REQUIRE(gctx.sends[{0, 1}].empty());
}
