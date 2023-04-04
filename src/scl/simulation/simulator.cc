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

#include "scl/simulation/simulator.h"

#include <chrono>
#include <cmath>
#include <exception>
#include <memory>
#include <ratio>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "scl/net/channel.h"
#include "scl/net/network.h"
#include "scl/protocol/base.h"
#include "scl/simulation/channel.h"
#include "scl/simulation/channel_id.h"
#include "scl/simulation/config.h"
#include "scl/simulation/context.h"
#include "scl/simulation/env.h"
#include "scl/simulation/event.h"
#include "scl/simulation/mem_channel_buffer.h"
#include "scl/simulation/result.h"
#include "scl/util/time.h"

namespace {

/**
 * @brief Create an Event of some type and duration.
 */
auto CreateEvent(scl::sim::Event::Type t, scl::util::Time::Duration d) {
  return std::make_shared<scl::sim::Event>(t, d);
}

std::vector<scl::net::Network> CreateNetworks(
    std::shared_ptr<scl::sim::SimulationContext> ctx) {
  std::vector<scl::net::Network> networks;
  const auto n = ctx->NumberOfParties();
  networks.reserve(n);

  for (std::size_t i = 0; i < n; ++i) {
    std::vector<std::shared_ptr<scl::net::Channel>> channels;
    channels.reserve(n);

    for (std::size_t j = 0; j < n; ++j) {
      scl::sim::ChannelId cid(i, j);
      channels.emplace_back(
          std::make_shared<scl::sim::SimulatedChannel>(cid, ctx));
    }

    networks.emplace_back(channels, i);
  }

  return networks;
}  // LCOV_EXCL_LINE

/**
 * @brief Create a SEGMENT_END or SEGMENT_BEGIN event.
 */
std::shared_ptr<scl::sim::Event> CreateSegmentEvent(scl::util::Time::Duration t,
                                                    const std::string& n,
                                                    bool is_end) {
  if (is_end) {
    return std::make_shared<scl::sim::SegmentEvent>(
        scl::sim::Event::Type::SEGMENT_END,
        t,
        n);
  }
  return std::make_shared<scl::sim::SegmentEvent>(
      scl::sim::Event::Type::SEGMENT_BEGIN,
      t,
      n);
}

/**
 * @brief Run a protocol step for a party.
 */
std::unique_ptr<scl::proto::Protocol> Run(
    std::shared_ptr<scl::sim::SimulationContext> ctx,
    std::size_t party_id,
    scl::proto::Protocol* party,
    scl::proto::ProtocolEnvironment& env,
    const scl::sim::OutputCallback& output_callback) {
  if (ctx->Trace(party_id).empty()) {
    ctx->AddEvent(party_id,
                  CreateEvent(scl::sim::Event::Type::START,
                              scl::util::Time::Duration::zero()));
  }

  ctx->AddEvent(
      party_id,
      CreateSegmentEvent(ctx->LatestTimestamp(party_id), party->Name(), false));

  ctx->UpdateCheckpoint();
  auto next = party->Run(env);
  const auto exec_time = ctx->Checkpoint(party_id);

  const auto output = party->Output();
  if (output.has_value()) {
    ctx->AddEvent(party_id,
                  CreateEvent(scl::sim::Event::Type::OUTPUT, exec_time));
    output_callback(party_id, output);
  }

  ctx->AddEvent(party_id, CreateSegmentEvent(exec_time, party->Name(), true));

  if (next == nullptr) {
    ctx->AddEvent(party_id,
                  CreateEvent(scl::sim::Event::Type::STOP, exec_time));
  }

  return next;
}

/**
 * @brief Run a simulation.
 */
std::vector<scl::sim::SimulationTrace> RunSimulation(
    std::vector<std::unique_ptr<scl::proto::Protocol>> protocols,
    const scl::sim::SimulatedNetworkConfigCreator& config_creator,
    const scl::sim::OutputCallback& output_callback) {
  const auto n = protocols.size();
  auto ps = std::move(protocols);

  auto ctx =
      scl::sim::SimulationContext::Create<scl::sim::MemoryBackedChannelBuffer>(
          n,
          config_creator);

  auto networks_ = CreateNetworks(ctx);

  auto next_id = ctx->NextToRun();
  while (next_id.has_value()) {
    auto id = next_id.value();

    try {
      ctx->Prepare(id);

      scl::proto::ProtocolEnvironment env{
          networks_[id],
          std::make_unique<scl::sim::SimulatedClock>(ctx.get(), id),
          std::make_unique<scl::sim::SimulatedThreadCtx>(ctx, id)};

      ps[id] = Run(ctx, id, ps[id].get(), env, output_callback);

      ctx->Commit(id);

    } catch (scl::sim::SimulationFailure& e) {
      ctx->Rollback(id);
    }

    next_id = ctx->NextToRun(id);
  }

  return ctx->Trace();
}

}  // namespace

std::vector<scl::sim::Result> scl::sim::Simulate(
    const ProtocolCreator& protocol_creator,
    const SimulatedNetworkConfigCreator& config_creator,
    std::size_t iterations,
    const OutputCallback& output_cb) {
  std::vector<std::vector<SimulationTrace>> traces;
  for (std::size_t i = 0; i < iterations; ++i) {
    traces.emplace_back(
        RunSimulation(protocol_creator(), config_creator, output_cb));
  }

  return Result::Create(traces);
}

std::vector<scl::sim::Result> scl::sim::Simulate(
    std::vector<std::unique_ptr<proto::Protocol>> parties,
    const SimulatedNetworkConfigCreator& config_creator,
    const OutputCallback& output_cb) {
  std::vector<std::vector<SimulationTrace>> traces;
  traces.emplace_back(
      RunSimulation(std::move(parties), config_creator, output_cb));
  return Result::Create(traces);
}
