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

#include "scl/coro/runtime.h"
#include "scl/coro/task.h"

using namespace scl;

namespace {

coro::Task<void> voidTask(bool& r) {
  r = true;
  co_return;
}

}  // namespace

TEST_CASE("void task", "[coro]") {
  bool set = false;
  auto rt = coro::DefaultRuntime::create();

  // Tasks are cold start, so not run before being executed by a runtime.
  auto void_task = voidTask(set);
  REQUIRE(!set);

  rt->run(std::move(void_task));

  REQUIRE(set);
}

namespace {

coro::Task<int> intTask() {
  co_return 42;
}

}  // namespace

TEST_CASE("int task", "[coro]") {
  auto rt = coro::DefaultRuntime::create();
  auto r = rt->run(intTask());
  REQUIRE(r == 42);
}

namespace {

coro::Task<int> anotherIntTask() {
  co_return co_await intTask() + 1;
}

coro::Task<int> adder() {
  auto v0 = co_await intTask();
  auto v1 = co_await anotherIntTask();
  co_return v0 + v1;
}

}  // namespace

TEST_CASE("adder task", "[coro]") {
  auto rt = coro::DefaultRuntime::create();
  // runs until the coroutine returns, even if it awaits.
  auto r = rt->run(adder());
  REQUIRE(r == 42 + 43);
}

namespace {

coro::Task<void> throws() {
  throw std::runtime_error("oops");
}

coro::Task<void> voidThrows() {
  co_await throws();
}

coro::Task<int> nonVoidThrows() {
  co_await throws();
  co_return 42;
}

}  // namespace

TEST_CASE("task throws void", "[coro]") {
  auto rt = coro::DefaultRuntime::create();
  REQUIRE_THROWS_MATCHES(rt->run(voidThrows()),
                         std::runtime_error,
                         Catch::Matchers::Message("oops"));
}

TEST_CASE("task throws non-void", "[coro]") {
  auto rt = coro::DefaultRuntime::create();
  REQUIRE_THROWS_MATCHES(rt->run(nonVoidThrows()),
                         std::runtime_error,
                         Catch::Matchers::Message("oops"));
}

TEST_CASE("result on unfinished Task", "[coro]") {
  auto t1 = nonVoidThrows();
  REQUIRE_THROWS_MATCHES(
      t1.result(),
      std::logic_error,
      Catch::Matchers::Message("result() called on unfinished coroutine"));
}
