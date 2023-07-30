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
#include "scl/simulation/simulator.h"

using namespace scl;
using namespace std::chrono_literals;

namespace {

template <typename T>
void ApproxDuration(util::Time::Duration d, T v, T b) {
  if (v > d) {
    REQUIRE(v - d <= b);
  } else {
    REQUIRE(d - v <= b);
  }
}

std::size_t KB(std::size_t bytes) {
  return 1000 * bytes;
}

std::size_t MB(std::size_t bytes) {
  return 1000 * KB(bytes);
}

}  // namespace

TEST_CASE("ComputeRecvTime default config", "[sim]") {
  // https://wintelguy.com/wanperf.pl
  // parameters:
  //  Link bandwidth (Mbit/s):            1
  //  RTT (millisecond):                  100
  //  Packet loss (%):                    0
  //  MTU (Byte):                         1500
  //  L1/L2 frame overhead (Byte):        0  <-- not accounted for in scl
  //  TCP/IP (v4) header overhead (Byte): 40
  //  TCP window (RWND) size (Byte):      65536
  //  File size (MByte):                  1

  const auto cfg = sim::ChannelConfig::Default();
  const auto tenMB = MB(10);
  const auto t = sim::ComputeRecvTime(cfg, tenMB);
  ApproxDuration(t, 82s, 1s);
}

TEST_CASE("ComputeRecvTime lossy", "[sim]") {
  const auto cfg = sim::ChannelConfig::Builder().PackageLoss(0.001).Build();
  const auto tenMB = MB(10);
  const auto t = sim::ComputeRecvTime(cfg, tenMB);
  ApproxDuration(t, 82s, 1s);
}

TEST_CASE("ComputeRecvTime lo", "[sim]") {
  const auto cfg = sim::ChannelConfig::Loopback();
  const auto amount = MB(10000);
  const auto t = sim::ComputeRecvTime(cfg, amount);
  REQUIRE(t.count() == 0);
}

TEST_CASE("SimulationConfig default", "[sim]") {
  auto cfg = sim::ChannelConfig::Default();

  REQUIRE(cfg.Bandwidth() == sim::ChannelConfig::DEFAULT_BANDWIDTH);
  REQUIRE(cfg.RTT() == sim::ChannelConfig::DEFAULT_RTT);
  REQUIRE(cfg.MSS() == sim::ChannelConfig::DEFAULT_MSS);
  REQUIRE(cfg.PackageLoss() == sim::ChannelConfig::DEFAULT_PACKAGE_LOSS);
  REQUIRE(cfg.WindowSize() == sim::ChannelConfig::DEFAULT_WINDOW_SIZE);
}

TEST_CASE("SimulationConfig setters", "[sim]") {
  auto cfg_it = sim::ChannelConfig::Builder{}.MSS(5000).Build();

  REQUIRE(cfg_it.MSS() == 5000);
  REQUIRE(cfg_it.Bandwidth() == sim::ChannelConfig::DEFAULT_BANDWIDTH);
  // Assume rest of properties are also defaulted correctly.
}

TEST_CASE("SimulationConfig validation", "[sim]") {
  REQUIRE_THROWS_MATCHES(sim::ChannelConfig::Builder{}.Bandwidth(0).Build(),
                         std::invalid_argument,
                         Catch::Matchers::Message("bandwidth cannot be 0"));

  REQUIRE_THROWS_MATCHES(sim::ChannelConfig::Builder{}.MSS(0).Build(),
                         std::invalid_argument,
                         Catch::Matchers::Message("MSS cannot be 0"));

  REQUIRE_THROWS_MATCHES(
      sim::ChannelConfig::Builder{}.PackageLoss(-0.1).Build(),
      std::invalid_argument,
      Catch::Matchers::Message("package loss percentage cannot be negative"));

  REQUIRE_THROWS_MATCHES(
      sim::ChannelConfig::Builder{}.PackageLoss(1).Build(),
      std::invalid_argument,
      Catch::Matchers::Message("package loss percentage cannot exceed 100%"));

  REQUIRE_THROWS_MATCHES(
      sim::ChannelConfig::Builder{}.WindowSize(0).Build(),
      std::invalid_argument,
      Catch::Matchers::Message("TCP window size cannot be 0"));
}

TEST_CASE("SimulationConfig to string", "[sim]") {
  std::stringstream ss;
  auto cfg = sim::ChannelConfig::Builder{}
                 .Bandwidth(2)
                 .MSS(10)
                 .RTT(50)
                 .PackageLoss(0.01)
                 .WindowSize(500)
                 .Build();
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
  auto cfg = sim::ChannelConfig::Loopback();
  REQUIRE(cfg.Type() == sim::ChannelConfig::NetworkType::INSTANT);
  std::stringstream ss;
  ss << cfg;
  REQUIRE(ss.str() == "SimulationConfig{INSTANT}");
}
