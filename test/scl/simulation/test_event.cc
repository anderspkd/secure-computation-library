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
#include <sstream>

#include "scl/simulation/event.h"

using namespace scl;
using namespace std::chrono_literals;

namespace {

auto str(std::shared_ptr<sim::Event> e) {
  std::stringstream s;
  s << e;
  return s.str();
}

}  // namespace

TEST_CASE("Simulation events", "[sim]") {
  SECTION("START") {
    auto e = sim::Event::start();
    REQUIRE(str(e) ==
            "{"
            "\"timestamp\":0,"
            "\"type\":\"START\","
            "\"metadata\":{}"
            "}");
  }

  SECTION("STOP") {
    auto e = sim::Event::stop(123ms);
    REQUIRE(str(e) ==
            "{"
            "\"timestamp\":123,"
            "\"type\":\"STOP\","
            "\"metadata\":{}"
            "}");
  }

  SECTION("CANCELLED") {
    auto e = sim::Event::cancelled(123ms);
    REQUIRE(str(e) ==
            "{"
            "\"timestamp\":123,"
            "\"type\":\"CANCELLED\","
            "\"metadata\":{}"
            "}");
  }

  SECTION("KILLED") {
    auto e = sim::Event::killed(123ms, "foo");
    REQUIRE(str(e) ==
            "{"
            "\"timestamp\":123,"
            "\"type\":\"KILLED\","
            "\"metadata\":{"
            "\"reason\":\"foo\""
            "}"
            "}");
  }

  SECTION("CLOSE") {
    auto e = sim::Event::closeChannel(123ms, {1, 2});
    REQUIRE(str(e) ==
            "{"
            "\"timestamp\":123,"
            "\"type\":\"CLOSE\","
            "\"metadata\":{"
            "\"channel_id\":{\"local\":1,\"remote\":2}"
            "}"
            "}");
  }

  SECTION("SEND") {
    auto e = sim::Event::sendData(123ms, {1, 2}, 10);
    REQUIRE(str(e) ==
            "{"
            "\"timestamp\":123,"
            "\"type\":\"SEND\","
            "\"metadata\":{"
            "\"channel_id\":{\"local\":1,\"remote\":2},"
            "\"amount\":10"
            "}"
            "}");
  }

  SECTION("RECV") {
    auto e = sim::Event::readData(123ms, {1, 2}, 10);
    REQUIRE(str(e) ==
            "{"
            "\"timestamp\":123,"
            "\"type\":\"READ\","
            "\"metadata\":{"
            "\"channel_id\":{\"local\":1,\"remote\":2},"
            "\"amount\":10"
            "}"
            "}");
  }

  SECTION("HAS_DATA") {
    auto e = sim::Event::hasData(123ms, {1, 2});
    REQUIRE(str(e) ==
            "{"
            "\"timestamp\":123,"
            "\"type\":\"HAS_DATA\","
            "\"metadata\":{"
            "\"channel_id\":{\"local\":1,\"remote\":2}"
            "}"
            "}");
  }

  SECTION("SLEEP") {
    auto e = sim::Event::sleep(123ms, 100ns);
    REQUIRE(str(e) ==
            "{"
            "\"timestamp\":123,"
            "\"type\":\"SLEEP\","
            "\"metadata\":{"
            "\"duration\":0.0001"
            "}"
            "}");
  }

  SECTION("OUTPUT") {
    auto e = sim::Event::output(123ms);
    REQUIRE(str(e) ==
            "{"
            "\"timestamp\":123,"
            "\"type\":\"OUTPUT\","
            "\"metadata\":{}"
            "}");
  }

  SECTION("PROTOCOL_BEGIN") {
    auto e = sim::Event::protocolBegin(123ms, "foo");
    REQUIRE(str(e) ==
            "{"
            "\"timestamp\":123,"
            "\"type\":\"PROTOCOL_BEGIN\","
            "\"metadata\":{"
            "\"name\":\"foo\""
            "}"
            "}");
  }

  SECTION("PROTOCOL_END") {
    auto e = sim::Event::protocolEnd(123ms, "foo");
    REQUIRE(str(e) ==
            "{"
            "\"timestamp\":123,"
            "\"type\":\"PROTOCOL_END\","
            "\"metadata\":{"
            "\"name\":\"foo\""
            "}"
            "}");
  }
}
