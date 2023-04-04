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
#include <iostream>
#include <sstream>

#include "scl/simulation/measurement.h"

using namespace scl;

TEST_CASE("Measurement to string", "[sim]") {
  sim::DataMeasurement dm;
  dm.AddSample(123.45);

  std::stringstream ss;
  ss << dm;
  REQUIRE(ss.str() == "{\"mean\": 123.45, \"unit\": \"B\", \"std_dev\": 0}");

  sim::TimeMeasurement tm;
  tm.AddSample(util::Time::Duration::zero());

  ss.str("");
  ss << tm;
  REQUIRE(ss.str() == "{\"mean\": 0, \"unit\": \"ms\", \"std_dev\": 0}");
}

TEST_CASE("Measurement data", "[sim]") {
  sim::DataMeasurement dm;
  dm.AddSample(2);
  dm.AddSample(4);
  dm.AddSample(4);
  dm.AddSample(4);
  dm.AddSample(5);
  dm.AddSample(5);
  dm.AddSample(7);
  dm.AddSample(9);

  REQUIRE(dm.Min() == 2);
  REQUIRE(dm.Max() == 9);
  REQUIRE(dm.Mean() == 5);
  REQUIRE(dm.Median() == 5);
  REQUIRE(dm.StdDev() == 2);
}

TEST_CASE("Measurement time", "[sim]") {
  using namespace std::chrono_literals;

  sim::TimeMeasurement tm;
  tm.AddSample(2ms);
  tm.AddSample(4ms);
  tm.AddSample(4ms);
  tm.AddSample(4ms);
  tm.AddSample(5ms);
  tm.AddSample(5ms);
  tm.AddSample(7ms);
  tm.AddSample(9ms);

  REQUIRE(tm.Min() == 2ms);
  REQUIRE(tm.Max() == 9ms);
  REQUIRE(tm.Mean() == 5ms);
  REQUIRE(tm.Median() == 5ms);
  REQUIRE(tm.StdDev() == 2ms);
}

TEST_CASE("Measurement median", "[sim]") {
  sim::DataMeasurement dm;
  REQUIRE(dm.Median() == 0);

  dm.AddSample(123);
  REQUIRE(dm.Median() == 123);
}

TEST_CASE("Measurement samples", "[sim]") {
  sim::DataMeasurement dm;
  REQUIRE(dm.Samples().empty());

  dm.AddSample(42);
  REQUIRE(dm.Size() == 1);
  REQUIRE(dm.Samples() == std::vector<long double>{42});

  dm.AddSample(22);
  REQUIRE(dm.Size() == 2);
  REQUIRE(dm.Samples() == std::vector<long double>{22, 42});
}
