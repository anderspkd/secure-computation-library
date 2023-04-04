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

#include "scl/simulation/context.h"

#include "scl/simulation/mem_channel_buffer.h"
#include "scl/simulation/simulator.h"

using SimCtx = scl::sim::SimulationContext;

template <>
std::shared_ptr<SimCtx> SimCtx::Create<scl::sim::MemoryBackedChannelBuffer>(
    std::size_t number_of_parties,
    const SimulatedNetworkConfigCreator& config) {
  auto ctx = std::make_shared<SimulationContext>(config);

  ctx->mNumberOfParties = number_of_parties;
  ctx->mTraces.resize(number_of_parties);
  ctx->mWritesIndices.resize(number_of_parties);

  for (std::size_t i = 0; i < number_of_parties; ++i) {
    ctx->mBuffers[ChannelId(i, i)] =
        MemoryBackedChannelBuffer::CreateLoopback();
    for (std::size_t j = i + 1; j < number_of_parties; ++j) {
      ChannelId cid(i, j);
      auto cp = MemoryBackedChannelBuffer::CreatePaired();
      ctx->mBuffers[cid] = cp[0];
      ctx->mBuffers[cid.Flip()] = cp[1];
    }
  }

  return ctx;
}  // LCOV_EXCL_LINE

namespace {

std::size_t Next(std::size_t id, std::size_t n) {
  return (id + 1) % n;
}

bool HasTerminated(const scl::sim::SimulationTrace& trace) {
  return !trace.empty() &&
         trace.back()->EventType() == scl::sim::Event::Type::STOP;
}

}  // namespace

std::optional<std::size_t> SimCtx::NextToRun(
    std::optional<std::size_t> current) {
  // party 0 is always the party to go first.
  if (!current.has_value()) {
    return 0;
  }

  // we end here current throw a SimulationFailure. This only happens when it
  // fails to either call Recv or HasData.
  if (mState == State::ROLLBACK) {
    // the last party in mNextPartyCandidates is assumed to be the party for
    // which current tried to Recv or HasData from.
    auto next = mNextPartyCandidates.back();

    // if this party has already finished, then current will never be able to
    // finish, so we crash the simulation here.
    if (HasTerminated(mTraces[next])) {
      throw SimulationFailure(
          "party tried to receive data from terminated party");
    }

    // if the party is the same as current, then we are performing a rollback
    // because we did not send enough data to ourselves. That data is never
    // going to arrive, so there's no hope of saving the simulation.
    if (next == current) {
      throw SimulationFailure("infinite loop detected");
    }

    return next;
  }

  // Last party ran succesfully, so we first attempt to pick the next party as
  // one of the parties that mCurrentParty sent data to.
  for (const auto next : mNextPartyCandidates) {
    if (next != current && !HasTerminated(mTraces[next])) {
      return next;
    }
  }

  std::size_t next = Next(current.value(), mNumberOfParties);
  std::size_t terminated = 0;
  while (terminated < mNumberOfParties) {
    if (!HasTerminated(mTraces[next])) {
      return next;
    }
    terminated++;
    next = Next(next, mNumberOfParties);
  }

  return {};
}

scl::util::Time::Duration SimCtx::Checkpoint(std::size_t id) {
  const auto latest = LatestTimestamp(id);
  const auto last_checkpoint = mCheckpoint;
  UpdateCheckpoint();
  return latest + (mCheckpoint - last_checkpoint);
}

void SimCtx::Prepare(std::size_t id) {
  if (mState == State::COMMIT || mState == State::ROLLBACK) {
    // The strategy used here assumes people are good at writing protocols and
    // so Rollbacks are relatively infrequent.
    mTraceIndex = mTraces[id].size();
    mNextPartyCandidates.clear();

    for (std::size_t i = 0; i < mNumberOfParties; ++i) {
      auto cid = ChannelId(id, i);
      mWritesIndices[i] = mWrites[cid].size();
      mBuffers[cid]->Prepare();
    }
  } else {
    throw std::logic_error("cannot prepare ctx");
  }
  mState = State::PREPARE;
}

void SimCtx::Commit(std::size_t id) {
  if (mState == State::PREPARE) {
    for (std::size_t i = 0; i < mNumberOfParties; ++i) {
      ChannelId cid(id, i);
      mBuffers[cid]->Commit();
    }

  } else {
    throw std::logic_error("cannot commit");
  }
  mState = State::COMMIT;
}

void SimCtx::Rollback(std::size_t id) {
  if (mState == State::PREPARE) {
    mTraces[id].resize(mTraceIndex);
    for (std::size_t i = 0; i < mWritesIndices.size(); ++i) {
      ChannelId cid(id, i);
      mWrites[cid].resize(mWritesIndices[i]);
      mBuffers[cid]->Rollback();
    }
  } else {
    throw std::logic_error("cannot rollback");
  }
  mState = State::ROLLBACK;
}
