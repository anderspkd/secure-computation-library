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
#include "scl/math/ff.h"
#include "scl/math/mersenne61.h"
#include "scl/net/loopback.h"
#include "scl/net/network.h"
#include "scl/protocol/env.h"
#include "scl/protocol/eval.h"
#include "scl/ss/additive.h"

using namespace scl;

using Elem = FF<Mersenne61>;

auto prg = PRG::create();
auto x = Elem(42);
auto y = Elem(11);
auto xs = additiveShare(x, 2, prg);
auto ys = additiveShare(y, 2, prg);
auto ts = test::randomTriple2<Elem>(prg);

namespace {

std::array<Env, 2> createEnvs() {
  auto p0p0 = LoopbackChannel::create();
  auto p1p1 = LoopbackChannel::create();
  auto p0p1 = LoopbackChannel::createPaired();

  return {createDefaultEnv(Network({p0p0, p0p1[0]}, 0)),
          createDefaultEnv(Network({p1p1, p0p1[1]}, 1))};
}

Task<Elem> runBeaverMulTwoParties() {
  auto envs = createEnvs();

  auto beaver0 = std::make_unique<test::BeaverMul<Elem>>(xs[0], ys[0], ts[0]);
  auto beaver1 = std::make_unique<test::BeaverMul<Elem>>(xs[1], ys[1], ts[1]);

  std::vector<Task<Elem>> protocol_evaluations;
  protocol_evaluations.emplace_back(
      evaluate<Elem>(std::move(beaver0), envs[0]));
  protocol_evaluations.emplace_back(
      evaluate<Elem>(std::move(beaver1), envs[1]));

  std::vector<Elem> shares = co_await batch(std::move(protocol_evaluations));

  co_return shares[0] + shares[1];
}

}  // namespace

TEST_CASE("Beaver multiplication protocol", "[proto]") {
  auto rt = DefaultRuntime::create();
  auto z = rt->run(runBeaverMulTwoParties());
  REQUIRE(z == x * y);
}
