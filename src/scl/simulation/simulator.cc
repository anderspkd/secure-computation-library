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
#include <thread>
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

using namespace scl;

namespace {

auto CreateEvent(sim::Event::Type t, util::Time::Duration d) {
  return std::make_shared<sim::Event>(t, d);
}

auto CreateSegmentEvent(util::Time::Duration t,
                        const std::string& n,
                        bool is_end) {
  if (is_end) {
    return std::make_shared<sim::SegmentEvent>(sim::Event::Type::SEGMENT_END,
                                               t,
                                               n);
  }
  return std::make_shared<sim::SegmentEvent>(sim::Event::Type::SEGMENT_BEGIN,
                                             t,
                                             n);
}

auto CreateNetworks(std::shared_ptr<sim::Context> ctx) {
  std::vector<net::Network> networks;
  const auto n = ctx->NumberOfParties();
  networks.reserve(n);

  for (std::size_t i = 0; i < n; ++i) {
    std::vector<std::shared_ptr<net::Channel>> channels;
    channels.reserve(n);

    for (std::size_t j = 0; j < n; ++j) {
      sim::ChannelId cid(i, j);
      channels.emplace_back(std::make_shared<sim::Channel>(cid, ctx));
    }

    networks.emplace_back(channels, i);
  }

  return networks;
}  // LCOV_EXCL_LINE

struct RunResult {
  std::unique_ptr<proto::Protocol> next;
  std::any output;
};

RunResult Run(std::shared_ptr<sim::Context> ctx,
              std::size_t id,
              proto::Protocol* protocol,
              proto::Env& env) {
  RunResult result;

  if (ctx->Trace(id).empty()) {
    ctx->AddEvent(
        id,
        CreateEvent(sim::Event::Type::START, util::Time::Duration::zero()));
  }

  if (protocol == nullptr) {
    // handling of entries which are null.
    ctx->AddEvent(
        id,
        CreateEvent(sim::Event::Type::STOP, util::Time::Duration::zero()));
    return result;
  }

  ctx->AddEvent(
      id,
      CreateSegmentEvent(ctx->LatestTimestamp(id), protocol->Name(), false));

  ctx->UpdateCheckpoint();
  result.next = protocol->Run(env);
  const auto exec_time = ctx->Checkpoint(id);

  result.output = protocol->Output();

  if (result.output.has_value()) {
    ctx->AddEvent(id, CreateEvent(sim::Event::Type::OUTPUT, exec_time));
  }

  ctx->AddEvent(id, CreateSegmentEvent(exec_time, protocol->Name(), true));

  if (result.next == nullptr) {
    ctx->AddEvent(id, CreateEvent(sim::Event::Type::STOP, exec_time));
  }

  return result;
}

std::vector<proto::Env> CreateEnvs(const std::vector<net::Network>& networks,
                                   std::shared_ptr<sim::Context> ctx) {
  std::vector<proto::Env> envs;
  envs.reserve(ctx->NumberOfParties());
  for (std::size_t i = 0; i < ctx->NumberOfParties(); ++i) {
    envs.emplace_back(proto::Env{networks[i],
                                 std::make_unique<sim::Clock>(ctx, i),
                                 std::make_unique<sim::ThreadCtx>(ctx, i)});
  }
  return envs;
}

auto RunSimulation(std::size_t replication, sim::Manager* manager) {
  auto ps = manager->Protocol();
  auto ctx = sim::Context::Create<sim::MemoryBackedChannelBuffer>(
      ps.size(),
      manager->NetworkConfiguration());

  auto networks = CreateNetworks(ctx);
  auto envs = CreateEnvs(networks, ctx);

  auto next_id = ctx->NextToRun();

  while (next_id.has_value()) {
    auto id = next_id.value();

    try {
      ctx->Prepare(id);

      auto result = Run(ctx, id, ps[id].get(), envs[id]);

      ps[id] = std::move(result.next);

      if (result.output.has_value()) {
        manager->HandleOutput(replication, id, result.output);
      }

      if (ps[id] != nullptr && manager->Terminate(id, ctx->GetView())) {
        ps[id] = nullptr;
        ctx->AddEvent(
            id,
            CreateEvent(sim::Event::Type::KILLED, ctx->LatestTimestamp(id)));
      }

      ctx->Commit(id);

    } catch (sim::SimulationFailure& e) {
      ctx->Rollback(id);
    }

    next_id = ctx->NextToRun(id);
  }

  return ctx->Trace();
}

}  // namespace

std::vector<sim::Result> sim::Simulate(std::unique_ptr<Manager> manager) {
  std::vector<std::vector<SimulationTrace>> traces;
  auto network_conf = manager->NetworkConfiguration();
  for (std::size_t i = 0; i < manager->Replications(); ++i) {
    traces.emplace_back(RunSimulation(i, manager.get()));
  }

  return Result::Create(traces);
}
