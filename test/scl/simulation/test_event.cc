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

#include "scl/simulation/event.h"

using namespace scl;

namespace {

std::string ToString(sim::Event* e) {
  std::stringstream ss;
  ss << e;
  return ss.str();
}

}  // namespace

TEST_CASE("Simulation Event", "[sim]") {
  SECTION("START") {
    sim::Event e(sim::Event::Type::START, util::Time::Duration::zero());
    REQUIRE(ToString(&e) == "START at 0 ms");
  }

  SECTION("STOP") {
    sim::Event e(sim::Event::Type::STOP, util::Time::Duration::zero());
    REQUIRE(ToString(&e) == "STOP at 0 ms");
  }

  const scl::sim::ChannelId cid{2, 5};

  SECTION("SEND") {
    sim::NetworkDataEvent e(sim::Event::Type::SEND,
                            util::Time::Duration::zero(),
                            cid,
                            100);
    REQUIRE(ToString(&e) == "SEND at 0 ms [Sender=2, Receiver=5, Amount=100]");
  }

  SECTION("PACKET_SEND") {
    sim::NetworkDataEvent e(sim::Event::Type::PACKET_SEND,
                            util::Time::Duration::zero(),
                            cid,
                            100);
    REQUIRE(ToString(&e) ==
            "PACKET_SEND at 0 ms [Sender=2, Receiver=5, Amount=100]");
  }

  SECTION("PACKET_RECV") {
    sim::PacketRecvEvent e(util::Time::Duration::zero(),
                           util::Time::Duration::zero(),
                           cid,
                           100,
                           false);
    REQUIRE(ToString(&e) ==
            "PACKET_RECV at 0 ms [Receiver=2, Sender=5, Amount=100, "
            "Blocking=false]");
  }

  SECTION("RECV") {
    sim::NetworkDataEvent e(sim::Event::Type::RECV,
                            util::Time::Duration::zero(),
                            cid,
                            100);
    REQUIRE(ToString(&e) == "RECV at 0 ms [Receiver=2, Sender=5, Amount=100]");
  }

  SECTION("HAS_DATA") {
    sim::HasDataEvent et(util::Time::Duration::zero(), cid, true);
    REQUIRE(ToString(&et) ==
            "HAS_DATA at 0 ms [Local=2, Remote=5, DataAvailable=true]");
    sim::HasDataEvent ef(util::Time::Duration::zero(), cid, false);
    REQUIRE(ToString(&ef) ==
            "HAS_DATA at 0 ms [Local=2, Remote=5, DataAvailable=false]");
  }

  SECTION("OUTPUT") {
    sim::Event e(sim::Event::Type::OUTPUT, util::Time::Duration::zero());
    REQUIRE(ToString(&e) == "OUTPUT at 0 ms");
  }

  SECTION("SLEEP") {
    sim::Event e(sim::Event::Type::SLEEP, util::Time::Duration::zero());
    REQUIRE(ToString(&e) == "SLEEP at 0 ms");
  }

  SECTION("SEGMENT_BEGIN") {
    sim::SegmentEvent e(sim::Event::Type::SEGMENT_BEGIN,
                        util::Time::Duration::zero(),
                        "foo");
    REQUIRE(ToString(&e) == "SEGMENT_BEGIN at 0 ms [Name=foo]");

    sim::SegmentEvent unnamed(sim::Event::Type::SEGMENT_BEGIN,
                              util::Time::Duration::zero(),
                              "");
    REQUIRE(ToString(&unnamed) == "SEGMENT_BEGIN at 0 ms [Unnamed segment]");
  }

  SECTION("SEGMENT_END") {
    sim::SegmentEvent e(sim::Event::Type::SEGMENT_END,
                        util::Time::Duration::zero(),
                        "foo");
    REQUIRE(ToString(&e) == "SEGMENT_END at 0 ms [Name=foo]");
  }

  SECTION("CLOSE") {
    sim::NetworkEvent e(sim::Event::Type::CLOSE,
                        util::Time::Duration::zero(),
                        cid);
    REQUIRE(ToString(&e) == "CLOSE at 0 ms [Local=2, Remote=5]");
  }

  SECTION("KILLED") {
    sim::Event e(sim::Event::Type::KILLED, util::Time::Duration::zero());
    REQUIRE(ToString(&e) == "KILLED at 0 ms");
  }

  SECTION("CHECKPOINT") {
    sim::CheckpointEvent e(util::Time::Duration::zero(), "asd");
    REQUIRE(ToString(&e) == "CHECKPOINT at 0 ms [asd]");
  }

  SECTION("With offset") {
    using namespace std::chrono_literals;
    sim::Event e(sim::Event::Type::START,
                 util::Time::Duration::zero(),
                 util::Time::Duration(123ms));
    REQUIRE(ToString(&e) == "START at 123 ms [Offset=123 ms]");
  }
}
