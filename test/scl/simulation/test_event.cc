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

TEST_CASE("Simulation Event write tests") {
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

TEST_CASE("EventList removes TRANSIENT events") {
  EventList evl;

  evl.add<TransientEvent>();

  REQUIRE(evl.size() == 1);
  REQUIRE(evl.latest()->type() == scl::EventType::TRANSIENT);

  evl.add<StartEvent>();

  // transient event should have been removed now

  REQUIRE(evl.size() == 1);
  REQUIRE(evl.latest()->type() == scl::EventType::START);
}
