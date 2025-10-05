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
#include <thread>

#include "scl/coro/batch.h"
#include "scl/coro/runtime.h"
#include "scl/coro/task.h"
#include "scl/net/config.h"
#include "scl/net/tcp/network.h"
#include "scl/time.h"

using namespace scl;

TEST_CASE("Network one party", "[net]") {
  auto rt = DefaultRuntime::create();
  auto network = rt->run(createTcpNetwork(NetworkConfig::localhost(0, 1)));
  REQUIRE(network.size() == 1);
}

namespace {

void connect(std::size_t id, Network* nw) {
  auto rt = DefaultRuntime::create();
  auto conf = NetworkConfig::localhost(id, 3);
  *nw = rt->run(createTcpNetwork(conf));
}

std::vector<Network> connect3() {
  std::vector<Network> networks(3);

  std::thread t0(connect, 0, &(networks[0]));
  std::thread t1(connect, 1, &(networks[1]));
  std::thread t2(connect, 2, &(networks[2]));

  t0.join();
  t1.join();
  t2.join();

  return networks;
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

  auto networks = connect3();

  REQUIRE(networks.size() == 3);

  rt->run(send(networks[0].party(1), 123));
  rt->run(send(networks[2].party(0), 456));

  auto v = rt->run(recv(networks[1].party(0)));
  REQUIRE(v == 123);

  auto w = rt->run(recv(networks[0].party(2)));
  REQUIRE(w == 456);
}

TEST_CASE("Network TCP timeout recv", "[net]") {
  using namespace std::chrono_literals;

  auto rt = DefaultRuntime::create();

  auto networks = connect3();

  RealtimeClock clock;

  rt->run(networks[0].party(1)->recv(500ms));

  REQUIRE(clock.read() > 500ms);
}
