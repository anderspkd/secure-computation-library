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

#include "scl/coro/runtime.h"
#include "scl/coro/task.h"
#include "scl/protocol.h"

using namespace scl;

namespace {

class CountdownProtocol final : public Protocol {
 public:
  CountdownProtocol(std::size_t n) : m_n(n) {}

  Task<Result> run(Env& /* ignored */) const override {
    if (m_n > 0) {
      co_return Result::nextStep(std::make_unique<CountdownProtocol>(m_n - 1),
                                 m_n);
    } else {
      co_return Result::done();
    }
  }

 private:
  std::size_t m_n;
};

}  // namespace

TEST_CASE("Protocol eval") {
  std::unique_ptr<Protocol> p = std::make_unique<CountdownProtocol>(10);

  auto rt = DefaultRuntime::create();

  Env env;

  std::size_t cd = 10;

  rt->run(runProtocol(std::move(p), env, [&cd](std::any& x) {
    REQUIRE(cd == std::any_cast<std::size_t>(x));
    cd--;
  }));
}
