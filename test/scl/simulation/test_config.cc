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
#include <sstream>
#include <stdexcept>

#include "scl/simulation/config.h"
#include "scl/util/time.h"

using namespace scl;
using namespace std::chrono_literals;

namespace {

template <typename T>
void approxDuration(util::Time::Duration d, T v, T b) {
  if (v > d) {
    REQUIRE(v - d <= b);
  } else {
    REQUIRE(d - v <= b);
  }
}

}  // namespace

TEST_CASE("SimulationConfig default", "[sim]") {
  auto cfg = sim::ChannelConfig::defaultConfig();

  REQUIRE(cfg.bandwidth() == sim::ChannelConfig::DEFAULT_BANDWIDTH);
  REQUIRE(cfg.RTT() == sim::ChannelConfig::DEFAULT_RTT);
  REQUIRE(cfg.MSS() == sim::ChannelConfig::DEFAULT_MSS);
  REQUIRE(cfg.packetLoss() == sim::ChannelConfig::DEFAULT_PACKAGE_LOSS);
  REQUIRE(cfg.windowSize() == sim::ChannelConfig::DEFAULT_WINDOW_SIZE);
}

TEST_CASE("SimulationConfig setters", "[sim]") {
  auto cfg_it = sim::ChannelConfig::Builder{}.MSS(5000).build();

  REQUIRE(cfg_it.MSS() == 5000);
  REQUIRE(cfg_it.bandwidth() == sim::ChannelConfig::DEFAULT_BANDWIDTH);
  // Assume rest of properties are also defaulted correctly.
}

TEST_CASE("SimulationConfig validation", "[sim]") {
  REQUIRE_THROWS_MATCHES(sim::ChannelConfig::Builder{}.bandwidth(0).build(),
                         std::invalid_argument,
                         Catch::Matchers::Message("bandwidth cannot be 0"));

  REQUIRE_THROWS_MATCHES(sim::ChannelConfig::Builder{}.MSS(0).build(),
                         std::invalid_argument,
                         Catch::Matchers::Message("MSS cannot be 0"));

  REQUIRE_THROWS_MATCHES(
      sim::ChannelConfig::Builder{}.packetLoss(-0.1).build(),
      std::invalid_argument,
      Catch::Matchers::Message("package loss percentage cannot be negative"));

  REQUIRE_THROWS_MATCHES(
      sim::ChannelConfig::Builder{}.packetLoss(1).build(),
      std::invalid_argument,
      Catch::Matchers::Message("package loss percentage cannot exceed 100%"));

  REQUIRE_THROWS_MATCHES(
      sim::ChannelConfig::Builder{}.windowSize(0).build(),
      std::invalid_argument,
      Catch::Matchers::Message("TCP window size cannot be 0"));
}

TEST_CASE("SimulationConfig to string", "[sim]") {
  std::stringstream ss;
  auto cfg = sim::ChannelConfig::Builder{}
                 .bandwidth(2)
                 .MSS(10)
                 .RTT(50)
                 .packetLoss(0.01)
                 .windowSize(500)
                 .build();
  ss << cfg;
  // clang-format off
  REQUIRE(ss.str() == "SimulationConfig{"
          "Type: TCP, "
          "Bandwidth: 2 bits/s, "
          "RTT: 50 ms, "
          "MSS: 10 bytes, "
          "PackageLoss: 1%, "
          "WindowSize: 500 bytes}");
  // clang-format on
}

TEST_CASE("SimulationConfig local", "[sim]") {
  auto cfg = sim::ChannelConfig::loopback();
  REQUIRE(cfg.type() == sim::ChannelConfig::NetworkType::INSTANT);
  std::stringstream ss;
  ss << cfg;
  REQUIRE(ss.str() == "SimulationConfig{INSTANT}");
}
