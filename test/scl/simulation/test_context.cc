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
#include <stdexcept>

#include "scl/simulation/channel_id.h"
#include "scl/simulation/config.h"
#include "scl/simulation/context.h"
#include "scl/simulation/event.h"

using namespace scl;
using namespace std::chrono_literals;

TEST_CASE("Context", "[sim]") {
  auto gctx = sim::details::GlobalContext::create(
      5,
      std::make_unique<sim::SimpleNetworkConfig>(),
      {});

  REQUIRE(gctx.traces.size() == 5);

  auto view0 = gctx.view(0);
  view0.recordEvent(sim::Event::start());
  view0.startClock();
  REQUIRE(view0.elapsedTime() > 0ms);

  view0.recordEvent(sim::Event::closeChannel(100ms, {0, 0}));
  auto view1 = gctx.view(1);
  REQUIRE(view1.currentTimeOf(0) == 100ms);
}

TEST_CASE("Context send", "[sim]") {
  auto gctx = sim::details::GlobalContext::create(
      5,
      std::make_unique<sim::SimpleNetworkConfig>(),
      {});

  auto view0 = gctx.view(0);
  view0.send(1, 100ms);
  REQUIRE(gctx.sends[{0, 1}].front() == 100ms);

  view0.send(1, 150ms);
  REQUIRE(gctx.sends[{0, 1}].front() == 100ms);
  gctx.sends[{0, 1}].pop_front();
  REQUIRE(gctx.sends[{0, 1}].front() == 150ms);
}

TEST_CASE("Context recv", "[sim]") {
  auto gctx = sim::details::GlobalContext::create(
      5,
      std::make_unique<sim::SimpleNetworkConfig>(),
      {});

  auto view0 = gctx.view(0);
  auto view1 = gctx.view(1);

  view0.send(1, 100ms);
  auto dur = view1.recv(0, 10, 100ms);
  REQUIRE(dur > 100ms);

  view0.send(1, 0ms);
  dur = view1.recv(0, 10, 1s);
  // data will already have arrived when recv is called.
  REQUIRE(dur == 1s);
}
