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
#include <sstream>
#include <stdexcept>

#include "scl/simulation/event.h"
#include "scl/simulation/result.h"

using namespace scl;

namespace {

std::shared_ptr<sim::Event> Stop() {
  return std::make_shared<sim::Event>(sim::Event::Type::STOP,
                                      util::Time::Duration::zero());
}

std::shared_ptr<sim::Event> Start() {
  return std::make_shared<sim::Event>(sim::Event::Type::START,
                                      util::Time::Duration::zero());
}

std::shared_ptr<sim::Event> BeginSegment(const std::string& name = "foo") {
  return std::make_shared<sim::SegmentEvent>(sim::Event::Type::SEGMENT_BEGIN,
                                             util::Time::Duration::zero(),
                                             name);
}

using TraceT = std::vector<std::vector<sim::SimulationTrace>>;

#define CREATE_TRACE(...) \
  {                       \
    {                     \
      { __VA_ARGS__ }     \
    }                     \
  }

}  // namespace

TEST_CASE("Simulation result invalid-traces", "[sim]") {
  TraceT trace_no_start = CREATE_TRACE(Stop());
  REQUIRE_THROWS_MATCHES(sim::Result::Create(trace_no_start),
                         std::logic_error,
                         Catch::Matchers::Message("incomplete trace"));

  TraceT trace_no_stop = CREATE_TRACE(Start());
  REQUIRE_THROWS_MATCHES(sim::Result::Create(trace_no_stop),
                         std::logic_error,
                         Catch::Matchers::Message("truncated trace"));

  TraceT trace_invalid_segment = CREATE_TRACE(Start(), BeginSegment(), Stop());

  REQUIRE_THROWS_MATCHES(sim::Result::Create(trace_invalid_segment),
                         std::logic_error,
                         Catch::Matchers::Message("incomplete segment"));
}

namespace {

std::shared_ptr<sim::Event> EndSegment(const std::string& name = "foo") {
  return std::make_shared<sim::SegmentEvent>(sim::Event::Type::SEGMENT_END,
                                             util::Time::Duration::zero(),
                                             name);
}

std::shared_ptr<sim::Event> Send(std::size_t to,
                                 std::size_t from,
                                 std::size_t amount) {
  return std::make_shared<sim::NetworkDataEvent>(sim::Event::Type::SEND,
                                                 util::Time::Duration::zero(),
                                                 sim::ChannelId{to, from},
                                                 amount);
}

std::shared_ptr<sim::Event> Recv(std::size_t to,
                                 std::size_t from,
                                 std::size_t amount) {
  return std::make_shared<sim::NetworkDataEvent>(sim::Event::Type::RECV,
                                                 util::Time::Duration::zero(),
                                                 sim::ChannelId{to, from},
                                                 amount);
}

}  // namespace

TEST_CASE("Simulation result sent recv", "[sim]") {
  TraceT trace = CREATE_TRACE(Start(),
                              BeginSegment(),
                              Send(0, 1, 123),
                              Recv(0, 2, 444),
                              EndSegment(),
                              BeginSegment("bar"),
                              Send(0, 3, 42),
                              Send(0, 1, 22),
                              EndSegment("bar"),
                              Stop());

  auto r = sim::Result::Create(trace);
  REQUIRE(r[0].TransferAmounts(2).sent.Mean() == 0);
  REQUIRE(r[0].TransferAmounts(2).recv.Mean() == 444);

  REQUIRE(r[0].TransferAmounts(1).sent.Mean() == 123 + 22);
  REQUIRE(r[0].TransferAmounts(1, "bar").sent.Mean() == 22);

  std::vector<std::size_t> expected = {1, 2, 3};
  REQUIRE_THAT(r[0].Interactions(), Catch::Matchers::UnorderedEquals(expected));
  std::vector<std::size_t> expected_bar = {1, 3};
  REQUIRE_THAT(r[0].Interactions("bar"),
               Catch::Matchers::UnorderedEquals(expected_bar));
}

TEST_CASE("Simulation result write", "[sim]") {
  // TODO: This doesn't really test anything besides that Write is
  // stable(-ish). Ideally, the test should check that the result is consistent
  // with the content of a file on disk, but that likely requires that Write is
  // deterministic, which is not the case base writes a ton of unordered maps.

  TraceT trace = CREATE_TRACE(Start(),
                              BeginSegment(),
                              Send(0, 1, 123),
                              Recv(0, 2, 444),
                              EndSegment(),
                              BeginSegment("bar"),
                              Send(0, 3, 42),
                              Send(0, 1, 22),
                              EndSegment("bar"),
                              Stop());

  auto r = sim::Result::Create(trace);

  std::stringstream ss0;
  std::stringstream ss1;
  r[0].Write(ss0);
  r[0].Write(ss1);
  REQUIRE(ss0.str() == ss1.str());
}
