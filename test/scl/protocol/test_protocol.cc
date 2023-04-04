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
#include <typeinfo>

#include "./beaver.h"
#include "./triple.h"
#include "scl/math/fp.h"
#include "scl/net/network.h"
#include "scl/protocol/base.h"
#include "scl/protocol/env.h"
#include "scl/ss/additive.h"

using namespace scl;

using FF = math::Fp<61>;

auto prg = util::PRG::Create();
auto xs = ss::AdditiveShare(FF(42), 2, prg);
auto ys = ss::AdditiveShare(FF(11), 2, prg);
auto ts = test::RandomTriple<FF>(prg);

namespace {

auto CreateEnv(net::Network& network) {
  return proto::ProtocolEnvironment{
      network,
      std::make_unique<proto::RealTimeClock>(),
      std::make_unique<proto::StlThreadContext>()};
}

}  // namespace

TEST_CASE("Dynamic protocol beaver step-by-step", "[protocol]") {
  auto networks = net::CreateMemoryBackedNetwork(2);

  std::unique_ptr<proto::Protocol> p0 =
      std::make_unique<test::BeaverMul<FF>::Init>(xs[0], ys[0], ts[0]);
  std::unique_ptr<proto::Protocol> p1 =
      std::make_unique<test::BeaverMul<FF>::Init>(xs[1], ys[1], ts[1]);

  auto env0 = CreateEnv(networks[0]);
  auto env1 = CreateEnv(networks[1]);

  p0 = p0->Run(env0);
  p1 = p1->Run(env1);

  REQUIRE(p0->Run(env0) == nullptr);
  REQUIRE(p1->Run(env1) == nullptr);

  auto z0 = std::any_cast<math::Vec<FF>>(p0->Output());
  auto z1 = std::any_cast<math::Vec<FF>>(p1->Output());

  REQUIRE(z0.Size() == 100);
  REQUIRE(z1.Size() == 100);

  REQUIRE(z0[0] + z1[0] == FF(42) * FF(11));
}

TEST_CASE("Dynamic protocol eval beaver", "[protocol]") {
  auto networks = net::CreateMemoryBackedNetwork(2);

  auto p0 = test::BeaverMul<FF>::Create(xs[0], ys[0], ts[0]);
  auto p1 = test::BeaverMul<FF>::Create(xs[1], ys[1], ts[1]);

  math::Vec<FF> z0;
  math::Vec<FF> z1;

  std::thread t0([&]() {
    proto::Evaluate(std::move(p0), networks[0], [&](const std::any& v) {
      z0 = std::any_cast<math::Vec<FF>>(v);
    });
  });
  std::thread t1([&]() {
    proto::Evaluate(std::move(p1), networks[1], [&](const std::any& v) {
      z1 = std::any_cast<math::Vec<FF>>(v);
    });
  });

  t0.join();
  t1.join();

  REQUIRE(z0.Size() == 100);
  REQUIRE(z1.Size() == 100);

  for (std::size_t i = 0; i < 100; ++i) {
    REQUIRE(z0[i] + z1[i] == FF(42) * FF(11));
  }
}

TEST_CASE("Dynamic protocol eval null protocol", "[protocol]") {
  auto networks = net::CreateMemoryBackedNetwork(1);
  proto::Evaluate(nullptr, networks[0]);
}

TEST_CASE("Protocol env real-time clock", "[protocol]") {
  proto::RealTimeClock clock;

  using namespace std::chrono_literals;
  std::this_thread::sleep_for(100ms);

  auto d = clock.Read();

  REQUIRE(d <= 110ms);
  REQUIRE(d >= 100ms);
}

TEST_CASE("Protocol env Stl thread context", "[protocol]") {
  proto::StlThreadContext ctx;

  using namespace std::chrono_literals;

  auto t0 = util::Time::Now();

  ctx.Sleep(100);

  auto t1 = util::Time::Now();

  REQUIRE(t1 - t0 >= 100ms);
}
