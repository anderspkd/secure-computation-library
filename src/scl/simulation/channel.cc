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

#include "scl/simulation/channel.h"

#include "scl/simulation/event.h"
#include "scl/simulation/simulator.h"

void scl::sim::SimulateClose(std::shared_ptr<SimulationContext> ctx,
                             ChannelId id) {
  const auto lid = id.local;
  const auto trt = ctx->Checkpoint(lid);
  const auto event = std::make_shared<Event>(Event::Type::CLOSE, trt);
  ctx->AddEvent(lid, event);
}

#define SCL_LOCAL_COMP_BEGIN const auto scl__lcb = scl::util::Time::Now()
#define SCL_LOCAL_COMP_END scl::util::Time::Now() - scl__lcb

void scl::sim::SimulateSend(std::shared_ptr<SimulationContext> ctx,
                            ChannelId id,
                            const unsigned char* src,
                            std::size_t n) {
  SCL_LOCAL_COMP_BEGIN;

  ctx->Buffer(id)->Write({src, src + n});

  const auto local_comp_time = SCL_LOCAL_COMP_END;
  const auto exec_time = ctx->Checkpoint(id.local) - local_comp_time;

  auto event = std::make_shared<NetworkEvent>(Event::Type::SEND,
                                              exec_time,
                                              id.local,
                                              id.remote,
                                              n);
  ctx->AddCandidateToRun(id.remote);
  ctx->AddEvent(id.local, event);
  ctx->RecordWrite(id, n, exec_time);
}

namespace {

scl::util::Time::Duration AdjustRecvTime(
    std::shared_ptr<scl::sim::SimulationContext> ctx,
    scl::sim::ChannelId id,
    scl::util::Time::Duration t,
    std::size_t n) {
  auto rem = n;
  auto wb = ctx->Writes(id).begin();
  auto we = ctx->Writes(id).end();

  while (rem > 0 && wb != we) {
    // TODO: It would probably be nicer to let ctx clean up the list of write
    // ops so this check isn't necessary.

    if (wb->amount == 0) {
      wb++;
      continue;
    }

    scl::util::Time::Duration recv_time;
    if (wb->amount >= rem) {
      const auto delay = scl::sim::ComputeRecvTime(ctx->NetworkConfig(id), rem);
      recv_time = wb->time + delay;
      wb->amount -= rem;
      rem = 0;
    } else /* wb->amount < rem */ {
      const auto delay =
          scl::sim::ComputeRecvTime(ctx->NetworkConfig(id), wb->amount);
      recv_time = wb->time + delay;
      rem -= wb->amount;
      wb->amount = 0;
    }

    t = std::max(t, recv_time);
    wb++;
  }

  return t;
}

}  // namespace

void scl::sim::SimulateRecv(std::shared_ptr<SimulationContext> ctx,
                            ChannelId id,
                            unsigned char* dst,
                            std::size_t n) {
  SCL_LOCAL_COMP_BEGIN;

  if (ctx->Buffer(id)->Size() < n) {
    ctx->AddCandidateToRun(id.remote);
    throw SimulationFailure();
  }

  auto data = ctx->Buffer(id)->Read(n);
  std::copy(data.begin(), data.end(), dst);

  const auto local_comp_time = SCL_LOCAL_COMP_END;
  const auto exec_time = ctx->Checkpoint(id.local) - local_comp_time;
  const auto adjusted_time = AdjustRecvTime(ctx, id.Flip(), exec_time, n);

  const auto event = std::make_shared<NetworkEvent>(Event::Type::RECV,
                                                    exec_time,
                                                    adjusted_time - exec_time,
                                                    id.local,
                                                    id.remote,
                                                    n);
  ctx->AddEvent(id.local, event);
}

bool scl::sim::SimulateHasData(std::shared_ptr<SimulationContext> ctx,
                               ChannelId id) {
  // The other party hasn't had a chance to run yet, so it's not possible to
  // determine if there's data available for us.
  if (ctx->Trace(id.remote).empty()) {
    ctx->AddCandidateToRun(id.remote);
    throw SimulationFailure();
  }

  const auto other_latest = ctx->LatestTimestamp(id.remote);
  const auto me_latest = ctx->Checkpoint(id.local);

  // The other party is still running, but is chronologically behind us, so it's
  // not possible to determine if there's data available for us.
  if (ctx->Trace(id.remote).back()->EventType() != Event::Type::STOP &&
      other_latest < me_latest) {
    ctx->AddCandidateToRun(id.remote);
    throw SimulationFailure();
  }

  // Check all handled writes of the other party. If there's one which took
  // place before me_latest that we haven't read yet, then there's data
  // available. Note that ordering of the writes do not matter here. We're just
  // interested in some unhandled write.
  bool has_data = false;
  for (const auto& wop : ctx->Writes(id.Flip())) {
    if (wop.time <= me_latest && wop.amount > 0) {
      has_data = true;
      break;
    }
  }

  return has_data;
}
