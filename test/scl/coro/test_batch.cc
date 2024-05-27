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
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <thread>

#include "scl/coro/batch.h"
#include "scl/coro/runtime.h"
#include "scl/coro/task.h"

using namespace scl;

namespace {

coro::Task<int> task() {
  co_return 42;
}

coro::Task<void> batch() {
  std::vector<coro::Task<int>> tasks;
  tasks.emplace_back(task());
  tasks.emplace_back(task());
  tasks.emplace_back(task());

  auto rs = co_await coro::batch(std::move(tasks));
  REQUIRE(rs.size() == 3);
  REQUIRE(rs[0] == 42);
  REQUIRE(rs[1] == 42);
  REQUIRE(rs[2] == 42);
}

}  // namespace

TEST_CASE("Simple batch", "[coro]") {
  auto rt = coro::DefaultRuntime::create();
  rt->run(batch());
}

namespace {

coro::Task<int> sleeps() {
  using namespace std::chrono_literals;
  co_await 100h;
  co_return 42;
}

coro::Task<void> partialBatch() {
  std::vector<coro::Task<int>> tasks;
  tasks.emplace_back(task());
  tasks.emplace_back(sleeps());
  tasks.emplace_back(task());

  auto rs = co_await coro::batch(std::move(tasks), 2);
  REQUIRE(rs.size() == 3);
  REQUIRE(rs[0].has_value());
  REQUIRE_FALSE(rs[1].has_value());
  REQUIRE(rs[2].has_value());

  REQUIRE(rs[0].value() == 42);
  REQUIRE(rs[2].value() == 42);
}

}  // namespace

TEST_CASE("Partial batch execution", "[coro]") {
  auto rt = coro::DefaultRuntime::create();
  rt->run(partialBatch());
}
