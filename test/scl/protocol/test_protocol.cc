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

#include "./beaver.h"
#include "./triple.h"
#include "scl/coro/runtime.h"
#include "scl/math/fp.h"
#include "scl/net/loopback.h"
#include "scl/net/network.h"
#include "scl/protocol/base.h"
#include "scl/protocol/env.h"
#include "scl/protocol/eval.h"
#include "scl/ss/additive.h"

using namespace scl;

using FF = math::Fp<61>;

auto prg = util::PRG::create();
auto x = FF(42);
auto y = FF(11);
auto xs = ss::additiveShare(x, 2, prg);
auto ys = ss::additiveShare(y, 2, prg);
auto ts = test::randomTriple2<FF>(prg);

namespace {

std::array<proto::Env, 2> createEnvs() {
  auto p0p0 = net::LoopbackChannel::create();
  auto p1p1 = net::LoopbackChannel::create();
  auto p0p1 = net::LoopbackChannel::createPaired();

  return {proto::createDefaultEnv(net::Network({p0p0, p0p1[0]}, 0)),
          proto::createDefaultEnv(net::Network({p1p1, p0p1[1]}, 1))};
}

coro::Task<FF> runBeaverMulTwoParties() {
  auto envs = createEnvs();

  auto beaver0 = std::make_unique<test::BeaverMul<FF>>(xs[0], ys[0], ts[0]);
  auto beaver1 = std::make_unique<test::BeaverMul<FF>>(xs[1], ys[1], ts[1]);

  std::vector<coro::Task<FF>> protocol_evaluations;
  protocol_evaluations.emplace_back(
      proto::evaluate<FF>(std::move(beaver0), envs[0]));
  protocol_evaluations.emplace_back(
      proto::evaluate<FF>(std::move(beaver1), envs[1]));

  std::vector<FF> shares =
      co_await coro::batch(std::move(protocol_evaluations));

  co_return shares[0] + shares[1];
}

}  // namespace

TEST_CASE("Beaver multiplication protocol", "[proto]") {
  auto rt = coro::DefaultRuntime::create();
  auto z = rt->run(runBeaverMulTwoParties());
  REQUIRE(z == x * y);
}
