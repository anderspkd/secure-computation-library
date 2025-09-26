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

#include "scl/coro/batch.h"
#include "scl/coro/runtime.h"
#include "scl/coro/task.h"
#include "scl/net/config.h"
#include "scl/net/tcp/network.h"

using namespace scl;

TEST_CASE("Network one party", "[net]") {
  auto rt = DefaultRuntime::create();
  auto network = rt->run(createTcpNetwork(NetworkConfig::localhost(0, 1)));
  REQUIRE(network.size() == 1);
}

namespace {

Task<std::vector<Network>> connect3() {
  std::vector<Task<Network>> networks;
  auto conf0 = NetworkConfig::localhost(0, 3);
  networks.emplace_back(createTcpNetwork(conf0));

  auto conf1 = NetworkConfig::localhost(1, 3);
  networks.emplace_back(createTcpNetwork(conf1));

  auto conf2 = NetworkConfig::localhost(2, 3);
  networks.emplace_back(createTcpNetwork(conf2));

  co_return co_await batch(std::move(networks));
}

Task<void> send(Channel* channel, int v) {
  Packet p;
  p << v;
  co_await channel->send(p);
}

Task<int> recv(Channel* channel) {
  Packet p = co_await channel->recv();
  co_return p.read<int>();
}

}  // namespace

TEST_CASE("Network TCP", "[net]") {
  auto rt = DefaultRuntime::create();

  auto networks = rt->run(connect3());

  REQUIRE(networks.size() == 3);

  rt->run(send(networks[0].party(1), 123));
  rt->run(send(networks[2].party(0), 456));

  auto v = rt->run(recv(networks[1].party(0)));
  REQUIRE(v == 123);

  auto w = rt->run(recv(networks[0].party(2)));
  REQUIRE(w == 456);
}
