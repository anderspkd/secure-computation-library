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
#include <stdexcept>

#include "scl/simulation/buffer.h"
#include "scl/simulation/channel_id.h"
#include "scl/simulation/config.h"
#include "scl/simulation/context.h"
#include "scl/simulation/event.h"
#include "scl/simulation/mem_channel_buffer.h"
#include "scl/simulation/simulator.h"

using namespace scl;

namespace {

auto SomeEvent() {
  return std::make_shared<sim::Event>(sim::Event::Type::START,
                                      util::Time::Duration::zero());
}

}  // namespace

TEST_CASE("Simulation context add events", "[sim]") {
  auto ctx = sim::SimulationContext::Create<sim::MemoryBackedChannelBuffer>(
      5,
      sim::DefaultConfigCreator());

  ctx->AddEvent(2, SomeEvent());
  ctx->AddEvent(2, SomeEvent());
  ctx->AddEvent(1, SomeEvent());

  const auto t = ctx->Trace();
  REQUIRE(t.size() == 5);
  REQUIRE(t[2].size() == 2);
  REQUIRE(t[1].size() == 1);
  REQUIRE(t[0].empty());
}

TEST_CASE("Simulation context total run time", "[sim]") {
  auto ctx = sim::SimulationContext::Create<sim::MemoryBackedChannelBuffer>(
      5,
      sim::DefaultConfigCreator());

  ctx->AddEvent(0, SomeEvent());
  auto t0 = ctx->Checkpoint(0);
  auto t1 = ctx->Checkpoint(0);

  // t0 is the difference between 0 and now (so very large). t1 is the
  // difference between t0 and now (so kinda small).
  REQUIRE(t0 > t1);
}

namespace {

struct DummyChannelBuffer final : public sim::ChannelBuffer {
  std::vector<unsigned char> Read(std::size_t n) override {
    (void)n;
    throw std::logic_error("not supported");
  }

  void Write(const std::vector<unsigned char>& data) override {
    (void)data;
    throw std::logic_error("not supported");
  }

  std::size_t Size() override {
    throw std::logic_error("not supported");
  }

  void Prepare() override {
    prepare_called++;
  }

  void Commit() override {
    commit_called++;
  }

  void Rollback() override {
    rollback_called++;
  }

  std::size_t prepare_called = 0;
  std::size_t commit_called = 0;
  std::size_t rollback_called = 0;
};

}  // namespace

namespace scl {

template <>
std::shared_ptr<sim::SimulationContext>
sim::SimulationContext::Create<DummyChannelBuffer>(
    std::size_t n,
    const sim::SimulatedNetworkConfigCreator& config_creator) {
  auto ctx = std::make_shared<sim::SimulationContext>(config_creator);

  ctx->mNumberOfParties = n;
  ctx->mTraces.resize(n);

  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t j = 0; j < n; ++j) {
      sim::ChannelId cid(i, j);
      ctx->mBuffers[cid] = std::make_shared<DummyChannelBuffer>();
    }
  }
  return ctx;
}

}  // namespace scl

#define AS_DUMMY(ctx, i, j)                      \
  std::dynamic_pointer_cast<DummyChannelBuffer>( \
      (ctx)->Buffer(sim::ChannelId((i), (j))))

TEST_CASE("Simulation context prepare-commit-rollback", "[sim]") {
  auto ctx = sim::SimulationContext::Create<DummyChannelBuffer>(
      5,
      sim::DefaultConfigCreator());

  ctx->Prepare(0);

  REQUIRE(AS_DUMMY(ctx, 0, 0)->prepare_called == 1);
  REQUIRE(AS_DUMMY(ctx, 0, 1)->prepare_called == 1);
  REQUIRE(AS_DUMMY(ctx, 0, 2)->prepare_called == 1);
  REQUIRE(AS_DUMMY(ctx, 0, 3)->prepare_called == 1);
  REQUIRE(AS_DUMMY(ctx, 0, 4)->prepare_called == 1);
  REQUIRE(AS_DUMMY(ctx, 1, 0)->prepare_called == 0);

  ctx->AddEvent(0, SomeEvent());
  ctx->AddEvent(0, SomeEvent());
  ctx->AddEvent(0, SomeEvent());

  REQUIRE(ctx->Trace()[0].size() == 3);

  ctx->Commit(0);
  REQUIRE(AS_DUMMY(ctx, 0, 0)->commit_called == 1);
  REQUIRE(AS_DUMMY(ctx, 0, 1)->commit_called == 1);
  REQUIRE(AS_DUMMY(ctx, 0, 2)->commit_called == 1);
  REQUIRE(AS_DUMMY(ctx, 0, 3)->commit_called == 1);
  REQUIRE(AS_DUMMY(ctx, 0, 4)->commit_called == 1);
  REQUIRE(AS_DUMMY(ctx, 1, 0)->commit_called == 0);

  REQUIRE(ctx->Trace()[0].size() == 3);

  ctx->Prepare(2);

  ctx->AddEvent(2, SomeEvent());
  ctx->AddEvent(2, SomeEvent());

  REQUIRE(ctx->Trace()[2].size() == 2);

  ctx->Rollback(2);
  REQUIRE(AS_DUMMY(ctx, 2, 0)->rollback_called == 1);
  REQUIRE(AS_DUMMY(ctx, 2, 1)->rollback_called == 1);
  REQUIRE(AS_DUMMY(ctx, 2, 2)->rollback_called == 1);
  REQUIRE(AS_DUMMY(ctx, 2, 3)->rollback_called == 1);
  REQUIRE(AS_DUMMY(ctx, 2, 4)->rollback_called == 1);
  REQUIRE(AS_DUMMY(ctx, 0, 1)->rollback_called == 0);

  REQUIRE(ctx->Trace()[2].empty());
  REQUIRE(ctx->Trace()[0].size() == 3);
}

TEST_CASE("Simulation context invalid prepare-commit-rollback", "[sim]") {
  auto ctx = sim::SimulationContext::Create<DummyChannelBuffer>(
      5,
      sim::DefaultConfigCreator());

  REQUIRE_THROWS_MATCHES(ctx->Commit(0),
                         std::logic_error,
                         Catch::Matchers::Message("cannot commit"));
  REQUIRE_THROWS_MATCHES(ctx->Rollback(0),
                         std::logic_error,
                         Catch::Matchers::Message("cannot rollback"));

  ctx->Prepare(0);

  REQUIRE_THROWS_MATCHES(ctx->Prepare(0),
                         std::logic_error,
                         Catch::Matchers::Message("cannot prepare ctx"));
}

namespace {

auto StopEvent() {
  return std::make_shared<sim::Event>(sim::Event::Type::STOP,
                                      util::Time::Duration::zero());
}

auto StartEvent() {
  return std::make_shared<sim::Event>(sim::Event::Type::START,
                                      util::Time::Duration::zero());
}

}  // namespace

TEST_CASE("Simulation context NextToRun simple", "[sim]") {
  auto ctx = sim::SimulationContext::Create<DummyChannelBuffer>(
      3,
      sim::DefaultConfigCreator());

  // First party to run is always party 0
  auto next = ctx->NextToRun();
  REQUIRE(next.has_value());
  REQUIRE(next.value() == 0);  // NOLINT

  ctx->AddEvent(0, StartEvent());
  ctx->AddEvent(1, StartEvent());
  ctx->AddEvent(2, StartEvent());

  // next to run is going to be party 1
  next = ctx->NextToRun(next);
  REQUIRE(next.has_value());
  REQUIRE(next.value() == 1);  // NOLINT

  // next would be party 2, but it has finished running, so party 0 is next.
  ctx->AddEvent(2, StopEvent());
  next = ctx->NextToRun(next);
  REQUIRE(next.has_value());
  REQUIRE(next.value() == 0);  // NOLINT

  ctx->AddEvent(0, StopEvent());
  next = ctx->NextToRun(next);
  REQUIRE(next.has_value());
  REQUIRE(next.value() == 1);  // NOLINT

  ctx->AddEvent(1, StopEvent());
  next = ctx->NextToRun(next);
  REQUIRE_FALSE(next.has_value());
}

TEST_CASE("Simulation context NextToRun fails", "[sim]") {
  auto ctx = sim::SimulationContext::Create<DummyChannelBuffer>(
      3,
      sim::DefaultConfigCreator());

  auto next = ctx->NextToRun();
  ctx->Prepare(0);

  ctx->AddEvent(0, StartEvent());
  ctx->AddCandidateToRun(2);

  ctx->Rollback(0);

  next = ctx->NextToRun(next);
  next = ctx->NextToRun(next);
  REQUIRE(next.has_value());
  REQUIRE(next.value() == 2);

  ctx->Prepare(2);

  // party 2 tries to receive from itself, but fails -- so it's gonna get stuck
  // in an infinite loop
  ctx->AddCandidateToRun(2);
  ctx->Rollback(2);

  REQUIRE_THROWS_MATCHES(ctx->NextToRun(next),
                         scl::sim::SimulationFailure,
                         Catch::Matchers::Message("infinite loop detected"));

  // Party 1 has stopped running, but party 2 expects data -- so party 2 will
  // never be able to finish running because the protocol is malformed.
  ctx->AddEvent(1, StopEvent());

  ctx->Prepare(2);
  ctx->AddCandidateToRun(1);

  ctx->Rollback(2);

  REQUIRE_THROWS_MATCHES(
      ctx->NextToRun(next),
      scl::sim::SimulationFailure,
      Catch::Matchers::Message(
          "party tried to receive data from terminated party"));
}

TEST_CASE("Simulation context rollback write ops", "[sim]") {
  auto ctx = sim::SimulationContext::Create<DummyChannelBuffer>(
      3,
      sim::DefaultConfigCreator());

  const auto ts = util::Time::Duration::zero();

  // party 0 sends to party 1
  ctx->Prepare(0);
  ctx->RecordWrite({0, 1}, 10, ts);
  ctx->Commit(0);

  // party 1 receives data from party 0, but then performs a rollback.
  ctx->Prepare(1);
  REQUIRE(ctx->Writes({0, 1})[0].amount == 10);
  ctx->Writes({0, 1})[0].amount = 0;
  ctx->Rollback(1);

  // the change to the write op above should be undone by the rollback.
  REQUIRE(ctx->Writes({0, 1})[0].amount == 10);
}
