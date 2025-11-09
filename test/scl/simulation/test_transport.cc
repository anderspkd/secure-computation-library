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

#include <scl/net/packet.h>

#include "scl/simulation/channel_id.h"
#include "scl/simulation/context.h"
#include "scl/simulation/network_description.h"
#include "scl/simulation/transport.h"

using namespace scl;
using namespace std::chrono_literals;

TEST_CASE("Transport send", "[sim]") {
  auto nd = NetworkDescription::createDefaultWAN(2);
  auto ctx = details::SimulatorContext::create(nd, {});
  ChannelId id{0, 1};

  details::Transport transport(ctx);

  Packet pkt;
  pkt << 123;

  transport.send(100ms, id, pkt);

  // from the receiver's pov
  REQUIRE(transport.ready(id.flip()));
  // from the sender's pov
  REQUIRE_FALSE(transport.ready(id));

  // receiving at time 0 should give a delay of ~100ms, since that's when the
  // packet was sent.
  auto [p, delay] = transport.recv(0s, id.flip());
  REQUIRE(p.read<int>() == 123);
  REQUIRE(delay >= 100ms);
  std::cout << delay << "\n";
}

TEST_CASE("Transport ready w. limit", "[sim]") {
  auto nd = NetworkDescription::createDefaultLAN(2);
  auto ctx = details::SimulatorContext::create(nd, {});
  ChannelId id{0, 1};
  auto sid = id.flip();

  details::Transport transport(ctx);

  Packet pkt;
  pkt << 123;

  REQUIRE_FALSE(transport.ready(sid, 101ms));

  transport.send(100ms, id, pkt);

  // the "limit" argument indicates the maximum allowed send time for a packet.
  REQUIRE_FALSE(transport.ready(sid, 99ms));
  REQUIRE(transport.ready(sid, 101ms));
}
