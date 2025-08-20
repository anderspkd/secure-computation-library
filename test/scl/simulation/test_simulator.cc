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

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <memory>
#include <ratio>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#include "scl/coro/coroutine.h"
#include "scl/protocol/base.h"
#include "scl/protocol/env.h"
#include "scl/protocol/result.h"
#include "scl/simulation/context.h"
#include "scl/simulation/event.h"
#include "scl/simulation/manager.h"
#include "scl/simulation/simulator.h"

using namespace scl;
using namespace std::chrono_literals;

TEST_CASE("Simulator no protocols", "[sim]") {
  struct NoProtocolManager final : public sim::Manager {
    std::vector<std::unique_ptr<proto::Protocol>> protocol() override {
      return {};
    }

    void handleSimulatorOutput(
        std::size_t /* ignored */,
        const sim::SimulationTrace& /* ignored */) override {
      FAIL();
    }
  };

  sim::simulate(std::make_unique<NoProtocolManager>());
}

// Protocol where each party sends a little bit of data to the other.
struct SendRecv final : public proto::Protocol {
  coro::Task<proto::ProtocolResult> run(proto::Env& env) const {
    net::Packet p;
    p << 1 << 2 << 3;

    co_await env.network.other()->send(p);
    auto r = co_await env.network.other()->recv();

    env.network.close();

    co_return proto::ProtocolResult::done();
  }
};

TEST_CASE("Simulate SendRecv protocol", "[sim]") {
  class SendRecvManager final : public sim::Manager {
   public:
    SendRecvManager(std::size_t n) : n_m(n) {}

    std::vector<std::unique_ptr<proto::Protocol>> protocol() override {
      std::vector<std::unique_ptr<proto::Protocol>> p;
      for (std::size_t i = 0; i < n_m; i++) {
        p.emplace_back(std::make_unique<SendRecv>());
      }
      return p;
    };

    void handleSimulatorOutput(std::size_t pid,
                               const sim::SimulationTrace& trace) override {
      if (pid == 0 || pid == 1) {
        validateTrace(trace);
      } else {
        FAIL();
      }
    }

   private:
    std::size_t n_m;

    void validateTrace(const sim::SimulationTrace& trace) {
      // start, begin, send, recv, close, close, end, stop
      REQUIRE(trace.size() == 8);
      REQUIRE(trace[0]->type == sim::EventType::START);
      REQUIRE(trace[1]->type == sim::EventType::PROTOCOL_BEGIN);
      REQUIRE(trace[2]->type == sim::EventType::SEND);
      REQUIRE(trace[3]->type == sim::EventType::READ);
      REQUIRE(trace[4]->type == sim::EventType::CLOSE);
      REQUIRE(trace[5]->type == sim::EventType::CLOSE);
      REQUIRE(trace[6]->type == sim::EventType::PROTOCOL_END);
      REQUIRE(trace[7]->type == sim::EventType::STOP);
    }
  };

  sim::simulate(std::make_unique<SendRecvManager>(2));
}

struct Sleepy final : public proto::Protocol {
  coro::Task<proto::ProtocolResult> run(proto::Env& /* ignored */) const {
    co_await 100s;
    co_return proto::ProtocolResult::done();
  }
};

TEST_CASE("Simulate Sleepy protocol", "[sim]") {
  struct SleepyManager final : public sim::Manager {
   public:
    std::vector<std::unique_ptr<proto::Protocol>> protocol() override {
      std::vector<std::unique_ptr<proto::Protocol>> p;
      p.emplace_back(std::make_unique<Sleepy>());
      return p;
    }

    void handleSimulatorOutput(std::size_t /* ignored */,
                               const sim::SimulationTrace& trace) override {
      REQUIRE(trace.size() == 5);
      REQUIRE(trace[0]->type == sim::EventType::START);
      REQUIRE(trace[1]->type == sim::EventType::PROTOCOL_BEGIN);
      REQUIRE(trace[2]->type == sim::EventType::SLEEP);
      REQUIRE(trace[3]->type == sim::EventType::PROTOCOL_END);
      REQUIRE(trace[4]->type == sim::EventType::STOP);
    }
  };

  sim::simulate(std::make_unique<SleepyManager>());
}

TEST_CASE("Simulate protocol cancellation", "[sim]") {
  struct CancelManager final : public sim::Manager {
    std::vector<std::unique_ptr<proto::Protocol>> protocol() override {
      std::vector<std::unique_ptr<proto::Protocol>> p;
      p.emplace_back(std::make_unique<Sleepy>());
      p.emplace_back(std::make_unique<Sleepy>());
      return p;
    }

    void handleSimulatorOutput(std::size_t /* ignored */,
                               const sim::SimulationTrace& trace) override {
      // there's two cases here: The party that gets to run first will cancel
      // the simulation at the first PROTOCOL_BEGIN event. The other party will
      // not get to run at all. We should thus see a trace with 3 events (START,
      // PROTOCOL_BEGIN, CANCELLED) and one with 0 events.

      if (trace.size() == 3) {
        REQUIRE(trace[0]->type == sim::EventType::START);
        REQUIRE(trace[1]->type == sim::EventType::PROTOCOL_BEGIN);
        REQUIRE(trace[2]->type == sim::EventType::CANCELLED);
      } else if (!trace.empty()) {
        FAIL("should not happen");
      }
    }
  };

  struct CancelHook : public sim::Hook {
    void run(std::size_t /* ignored */, const sim::SimulationContext& ctx) {
      ctx.cancelSimulation();
    }
  };

  auto man = std::make_unique<CancelManager>();
  man->addHook<CancelHook>(sim::EventType::PROTOCOL_BEGIN);

  sim::simulate(std::move(man));
}
