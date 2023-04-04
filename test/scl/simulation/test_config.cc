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
#include <sstream>
#include <stdexcept>

#include "scl/simulation/config.h"

using namespace scl;

TEST_CASE("SimulationConfig default", "[sim]") {
  auto cfg = sim::SimulatedNetworkConfig::Default();

  REQUIRE(cfg.Bandwidth() == sim::SimulatedNetworkConfig::kDefaultBandwidth);
  REQUIRE(cfg.RTT() == sim::SimulatedNetworkConfig::kDefaultRTT);
  REQUIRE(cfg.MSS() == sim::SimulatedNetworkConfig::kDefaultMSS);
  REQUIRE(cfg.PackageLoss() ==
          sim::SimulatedNetworkConfig::kDefaultPackageLoss);
  REQUIRE(cfg.WindowSize() == sim::SimulatedNetworkConfig::kDefaultWindowSize);
}

TEST_CASE("SimulationConfig setters", "[sim]") {
  auto cfg_it = sim::SimulatedNetworkConfig::Builder{}.MSS(5000).Build();

  REQUIRE(cfg_it.MSS() == 5000);
  REQUIRE(cfg_it.Bandwidth() == sim::SimulatedNetworkConfig::kDefaultBandwidth);
  // Assume rest of properties are also defaulted correctly.
}

TEST_CASE("SimulationConfig validation", "[sim]") {
  REQUIRE_THROWS_MATCHES(
      sim::SimulatedNetworkConfig::Builder{}.Bandwidth(0).Build(),
      std::invalid_argument,
      Catch::Matchers::Message("bandwidth cannot be 0"));

  REQUIRE_THROWS_MATCHES(sim::SimulatedNetworkConfig::Builder{}.MSS(0).Build(),
                         std::invalid_argument,
                         Catch::Matchers::Message("MSS cannot be 0"));

  REQUIRE_THROWS_MATCHES(
      sim::SimulatedNetworkConfig::Builder{}.PackageLoss(-0.1).Build(),
      std::invalid_argument,
      Catch::Matchers::Message("package loss percentage cannot be negative"));

  REQUIRE_THROWS_MATCHES(
      sim::SimulatedNetworkConfig::Builder{}.PackageLoss(1).Build(),
      std::invalid_argument,
      Catch::Matchers::Message("package loss percentage cannot exceed 100%"));

  REQUIRE_THROWS_MATCHES(
      sim::SimulatedNetworkConfig::Builder{}.WindowSize(0).Build(),
      std::invalid_argument,
      Catch::Matchers::Message("TCP window size cannot be 0"));
}

TEST_CASE("SimulationConfig to string", "[sim]") {
  std::stringstream ss;
  auto cfg = sim::SimulatedNetworkConfig::Builder{}
                 .Bandwidth(2)
                 .MSS(10)
                 .RTT(50)
                 .PackageLoss(0.01)
                 .WindowSize(500)
                 .Build();
  ss << cfg;
  // clang-format off
  REQUIRE(ss.str() == "SimulationConfig{"
          "Bandwidth: 2 bits/s, "
          "RTT: 50 ms, "
          "MSS: 10 bytes, "
          "PackageLoss: 1%, "
          "WindowSize: 500 bytes}");
  // clang-format on
}
