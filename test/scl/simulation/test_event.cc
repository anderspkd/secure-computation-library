/* SCL --- Secure Computation Library
 * Copyright (C) 2025 Anders Dalskov
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
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "scl/simulation/channel_id.h"
#include "scl/simulation/event.h"

using namespace scl;
using namespace std::chrono_literals;

TEST_CASE("Event write tests", "[sim]") {
  EventList evl;

  evl.add<StartEvent>();
  evl.add<StopEvent>(123s);
  evl.add<BeginEvent>(123s, "foo");
  evl.add<EndEvent>(123s, "bar");
  evl.add<KilledEvent>(123s, "fubar");
  evl.add<CancelledEvent>(123s);
  evl.add<CloseEvent>(123s, ChannelId{1, 2});
  evl.add<SendEvent>(123s, ChannelId{1, 2}, 50);
  evl.add<RecvEvent>(123s, ChannelId{1, 2}, 100);
  evl.add<RecvTimeoutEvent>(123s, ChannelId{1, 2});
  evl.add<PollEvent>(123s, ChannelId{1, 2}, true);
  evl.add<SleepEvent>(123s, 5s);

  std::string line;
  std::ifstream expected(SCL_TEST_DATA_DIR "events_write.json");

  if (!expected.is_open()) {
    throw std::runtime_error("could not open file");
  }

  for (const auto& e : evl) {
    std::stringstream ss;
    e->write(ss);
    std::getline(expected, line);
    REQUIRE(ss.str() == line);
  }
}

namespace {

struct TransientEvent final : public scl::Event {
  TransientEvent() : scl::Event{Time::Duration::zero()} {}
  void write(std::ostream&) override {}
  EventType type() const override {
    return scl::EventType::TRANSIENT;
  }
};

}  // namespace

TEST_CASE("EventList removes TRANSIENT events", "[sim]") {
  EventList evl;

  evl.add<TransientEvent>();

  REQUIRE(evl.size() == 1);
  REQUIRE(evl.latest()->type() == scl::EventType::TRANSIENT);

  evl.add<StartEvent>();

  // the TRANSIENT event should have been removed now

  REQUIRE(evl.size() == 1);
  REQUIRE(evl.latest()->type() == scl::EventType::START);

  // non-transient events are not removed

  evl.add<StopEvent>(10s);

  REQUIRE(evl.size() == 2);
  REQUIRE(evl.latest()->type() == scl::EventType::STOP);
}
