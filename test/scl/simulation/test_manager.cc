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
#include <memory>

#include "scl/simulation/config.h"
#include "scl/simulation/context.h"
#include "scl/simulation/manager.h"
#include "scl/simulation/mem_channel_buffer.h"

using namespace scl;

TEST_CASE("SingleReplicationManager", "[sim]") {
  sim::SingleReplicationManager m({});

  auto p = m.Protocol();
  REQUIRE(p.empty());

  REQUIRE_THROWS_MATCHES(
      m.Protocol(),
      std::logic_error,
      Catch::Matchers::Message(
          "Protocol called twice on SingleReplicationManager"));
}

struct DummyManager final : public sim::Manager {
  DummyManager() : sim::Manager(1) {}
  std::vector<std::unique_ptr<proto::Protocol>> Protocol() override {
    return {};
  }
};

TEST_CASE("Default Manager methods", "[sim]") {
  DummyManager m;

  auto ctx = sim::Context::Create<sim::MemoryBackedChannelBuffer>(1, nullptr);
  REQUIRE_FALSE(m.Terminate(0, ctx->GetView()));

  // checks that the config returned is of type SimpleNetworkConfig.
  auto p = std::dynamic_pointer_cast<sim::SimpleNetworkConfig>(
      m.NetworkConfiguration());
  REQUIRE(p != nullptr);
}
