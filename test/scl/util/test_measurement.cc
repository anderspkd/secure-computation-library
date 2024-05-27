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
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <iostream>
#include <sstream>

#include "scl/util/measurement.h"

using namespace scl;
using namespace std::chrono_literals;

TEST_CASE("Measurement to string", "[util]") {
  util::DataMeasurement dm;
  dm.addSample(123.45);

  std::stringstream ss;
  ss << dm;
  REQUIRE(ss.str() == "{\"mean\": 123.45, \"unit\": \"B\", \"std_dev\": 0}");

  util::TimeMeasurement tm;
  tm.addSample(util::Time::Duration::zero());

  ss.str("");
  ss << tm;
  REQUIRE(ss.str() == "{\"mean\": 0, \"unit\": \"ms\", \"std_dev\": 0}");
}

TEST_CASE("Measurement mean and stddev", "[util]") {
  util::DataMeasurement dm;
  dm.addSample(123.42);
  dm.addSample(555.21);
  REQUIRE_THAT(dm.mean(), Catch::Matchers::WithinRel(339.315, 0.001));
  REQUIRE_THAT(dm.stddev(), Catch::Matchers::WithinRel(305.322, 0.001));

  util::TimeMeasurement tm;
  tm.addSample(123ms);
  tm.addSample(444ms);
  REQUIRE(tm.mean() == 283.5ms);
  REQUIRE(tm.stddev() == 226.981276ms);
}

TEST_CASE("Measurement data", "[util]") {
  util::DataMeasurement dm;
  dm.addSample(2);
  dm.addSample(4);
  dm.addSample(4);
  dm.addSample(4);
  dm.addSample(5);
  dm.addSample(5);
  dm.addSample(7);
  dm.addSample(9);

  REQUIRE(dm.size() == 8);
  REQUIRE(dm.samples() == std::vector<long double>({2, 4, 4, 4, 5, 5, 7, 9}));
}

TEST_CASE("Measurement time", "[util]") {
  util::TimeMeasurement tm;
  tm.addSample(2ms);
  tm.addSample(4ms);
  tm.addSample(4ms);
  tm.addSample(4ms);
  tm.addSample(5ms);
  tm.addSample(5ms);
  tm.addSample(7ms);
  tm.addSample(9ms);

  REQUIRE(tm.size() == 8);
  REQUIRE(tm.samples() == std::vector<util::Time::Duration>(
                              {2ms, 4ms, 4ms, 4ms, 5ms, 5ms, 7ms, 9ms}));
}

TEST_CASE("Measurement samples", "[util]") {
  util::DataMeasurement dm;
  REQUIRE(dm.samples().empty());

  dm.addSample(42);
  REQUIRE(dm.size() == 1);
  REQUIRE(dm.samples() == std::vector<long double>{42});

  dm.addSample(22);
  REQUIRE(dm.size() == 2);
  REQUIRE(dm.samples() == std::vector<long double>{42, 22});
}

TEST_CASE("Measurement median", "[util]") {
  util::DataMeasurement dm;
  util::TimeMeasurement tm;

  REQUIRE(dm.median() == 0);
  REQUIRE(tm.median() == 0s);

  dm.addSample(123);
  tm.addSample(123s);

  REQUIRE(dm.median() == 123);
  REQUIRE(tm.median() == 123s);

  dm.addSample(442);
  tm.addSample(442s);

  REQUIRE(dm.median() == 282.5);
  REQUIRE(tm.median() == 282.5s);
}
