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
#include <thread>

#include "scl/net/shared_deque.h"

using namespace scl;

TEST_CASE("SharedDeque", "[misc]") {
  net::SharedDeque<int> dq;

  dq.PushBack(4);
  dq.PushBack(5);
  dq.PushBack(2);

  REQUIRE(dq.Peek() == 4);
  REQUIRE(dq.Peek() == 4);

  auto four = dq.Pop();
  REQUIRE(four == 4);

  REQUIRE(dq.Peek() == 5);
  dq.PopFront();
  auto two = dq.Pop();
  REQUIRE(two == 2);

  REQUIRE(dq.Size() == 0);
}

TEST_CASE("SharedDeque pop", "[misc]") {
  using namespace std::chrono_literals;

  SECTION("Pop") {
    net::SharedDeque<int> dq;
    int v = 0;

    std::thread t([&]() { v = dq.Pop(); });

    std::this_thread::sleep_for(20ms);

    REQUIRE(dq.Size() == 0);
    dq.PushBack(42);

    t.join();
    REQUIRE(dq.Size() == 0);
    REQUIRE(v == 42);
  }

  SECTION("PopFront") {
    net::SharedDeque<int> dq;

    std::thread t([&]() { dq.PopFront(); });

    std::this_thread::sleep_for(20ms);

    REQUIRE(dq.Size() == 0);
    dq.PushBack(42);

    t.join();
    REQUIRE(dq.Size() == 0);
  }
}
