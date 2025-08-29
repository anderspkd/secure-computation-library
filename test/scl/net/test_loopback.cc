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

#include "scl/coro/batch.h"
#include "scl/coro/runtime.h"
#include "scl/coro/task.h"
#include "scl/net/loopback.h"

using namespace scl;

TEST_CASE("Loopback to self close", "[net]") {
  auto channel = LoopbackChannel::create();

  Packet p;
  p << 1 << 2 << 3;

  auto rt = DefaultRuntime::create();

  rt->run(channel->send(p));
  auto received = rt->run(channel->recv());

  REQUIRE(received.read<int>() == 1);
  REQUIRE(received.read<int>() == 2);
  REQUIRE(received.read<int>() == 3);
}

TEST_CASE("Loopback send/recv", "[net]") {
  auto channels = LoopbackChannel::createPaired();
  auto chl0 = channels[0];
  auto chl1 = channels[1];

  Packet p;
  p << 1 << 2 << 3;

  auto rt = DefaultRuntime::create();

  rt->run(chl0->send(p));
  auto received = rt->run(chl1->recv());

  REQUIRE(received.read<int>() == 1);
  REQUIRE(received.read<int>() == 2);
  REQUIRE(received.read<int>() == 3);

  rt->run(chl1->send(std::move(p)));
  auto received1 = rt->run(chl0->recv());
  REQUIRE(received1.read<int>() == 1);
  REQUIRE(received1.read<int>() == 2);
  REQUIRE(received1.read<int>() == 3);
}
