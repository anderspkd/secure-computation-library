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

#include <any>
#include <catch2/catch_test_macros.hpp>
#include <initializer_list>
#include <memory>

#include "scl/coro.h"
#include "scl/protocol.h"
#include "scl/simulation/event.h"
#include "scl/simulation/params.h"
#include "scl/simulation/simulator.h"

using namespace scl;
using namespace std::chrono_literals;

namespace {

class PingPongProtocol final : public Protocol {
 public:
  PingPongProtocol(bool is_sender) : m_is_sender(is_sender) {}

  Task<Result> run(Env& env) const override {
    if (m_is_sender) {
      Packet pkt;
      pkt << 123;
      co_await 100ms;
      co_await env.network.other()->send(pkt);
    } else {
      auto pkt = co_await env.network.other()->recv();
    }

    co_return Result::done();
  }

 private:
  bool m_is_sender;
};

void assertEvents(EventList& event_list,
                  std::initializer_list<EventType> events) {
  REQUIRE(events.size() == event_list.size());

  auto got_it = event_list.cbegin();
  auto exp_it = events.begin();

  while (got_it != event_list.cend()) {
    REQUIRE((*got_it++)->type() == (*exp_it++));
  }
}

}  // namespace

TEST_CASE("Simulator test", "[sim]") {
  auto nd = NetworkParams::create(2);

  Simulator sim;
  auto res = sim.run(
      []() {
        std::vector<std::unique_ptr<Protocol>> p;
        p.emplace_back(std::make_unique<PingPongProtocol>(false));
        p.emplace_back(std::make_unique<PingPongProtocol>(true));
        return p;
      },
      nd);

  REQUIRE(res.numberOfParties() == 2);

  // validate the trace of party 0.
  // should be START, BEGIN, RECV, END, STOP
  assertEvents(res[0],
               {EventType::START,
                EventType::BEGIN,
                EventType::CHANNEL_RECV,
                EventType::END,
                EventType::STOP});

  // validate trace of party 1
  // should be START, BEGIN, SLEEP, SEND, END, STOP
  assertEvents(res[1],
               {EventType::START,
                EventType::BEGIN,
                EventType::SLEEP,
                EventType::CHANNEL_SEND,
                EventType::END,
                EventType::STOP});
}

namespace {

struct OutputProtocol final : public Protocol {
  Task<Result> run(Env& /* ignored */) const override {
    co_return Result::done(123);
  }
};

}  // namespace

TEST_CASE("Simulator handle output", "[sim]") {
  auto nd = NetworkParams::create(1);
  Simulator sim;

  bool called = false;

  sim.addOutputHandler([&called](std::size_t pid, std::any output) {
    called = (pid == 0) && (output.has_value()) &&
             (std::any_cast<int>(output) == 123);
  });

  auto res = sim.run(
      []() {
        std::vector<std::unique_ptr<Protocol>> p;
        p.emplace_back(std::make_unique<OutputProtocol>());
        return p;
      },
      nd);

  REQUIRE(called);
}
