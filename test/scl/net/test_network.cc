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
#include <cstdint>
#include <thread>

#include "scl/coro/batch.h"
#include "scl/coro/coroutine.h"
#include "scl/net/config.h"
#include "scl/net/network.h"
#include "scl/net/tcp_channel.h"

using namespace scl;

TEST_CASE("Network one party", "[net]") {
  auto rt = coro::DefaultRuntime::create();
  auto network =
      rt->run(net::Network::create(net::NetworkConfig::localhost(0, 1)));
  REQUIRE(network.size() == 1);
}

namespace {

coro::Task<std::vector<net::Network>> connect3() {
  std::vector<coro::Task<net::Network>> networks;
  auto conf0 = net::NetworkConfig::localhost(0, 3);
  networks.emplace_back(net::Network::create(conf0));

  auto conf1 = net::NetworkConfig::localhost(1, 3);
  networks.emplace_back(net::Network::create(conf1));

  auto conf2 = net::NetworkConfig::localhost(2, 3);
  networks.emplace_back(net::Network::create(conf2));

  co_return co_await coro::batch(std::move(networks));
}

coro::Task<void> send(net::Channel* channel, int v) {
  net::Packet p;
  p << v;
  co_await channel->send(p);
}

coro::Task<int> recv(net::Channel* channel) {
  net::Packet p = co_await channel->recv();
  co_return p.read<int>();
}

}  // namespace

TEST_CASE("Network TCP", "[net]") {
  auto rt = coro::DefaultRuntime::create();

  auto networks = rt->run(connect3());

  REQUIRE(networks.size() == 3);

  rt->run(send(networks[0].party(1), 123));
  rt->run(send(networks[2].party(0), 456));

  auto v = rt->run(recv(networks[1].party(0)));
  REQUIRE(v == 123);

  auto w = rt->run(recv(networks[0].party(2)));
  REQUIRE(w == 456);
}
