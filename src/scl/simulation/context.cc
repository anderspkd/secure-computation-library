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

#include "scl/simulation/config.h"
#include "scl/simulation/event.h"
#include "scl/simulation/mem_channel_buffer.h"
#include "scl/simulation/simulator.h"

using namespace scl;

template <>
std::shared_ptr<sim::Context>
sim::Context::Create<sim::MemoryBackedChannelBuffer>(
    std::size_t number_of_parties,
    std::shared_ptr<sim::NetworkConfig> config) {
  auto ctx = std::make_shared<Context>(config);

  ctx->m_nparties = number_of_parties;
  ctx->m_traces.resize(number_of_parties);

  for (std::size_t i = 0; i < number_of_parties; ++i) {
    ctx->m_buffers[ChannelId(i, i)] =
        MemoryBackedChannelBuffer::CreateLoopback();
    for (std::size_t j = i + 1; j < number_of_parties; ++j) {
      ChannelId cid(i, j);
      auto cp = MemoryBackedChannelBuffer::CreatePaired();
      ctx->m_buffers[cid] = cp[0];
      ctx->m_buffers[cid.Flip()] = cp[1];
    }
  }

  return ctx;
}  // LCOV_EXCL_LINE

namespace {

std::size_t Next(std::size_t id, std::size_t n) {
  return (id + 1) % n;
}

}  // namespace

std::optional<std::size_t> sim::Context::NextToRun(
    std::optional<std::size_t> current) {
  // party 0 is always the party to go first.
  if (!current.has_value()) {
    return 0;
  }

  // we end here current throw a SimulationFailure. This only happens when it
  // fails to either call Recv or HasData.
  if (m_state == State::ROLLBACK) {
    // the last party in m_next_party_cand is assumed to be the party for
    // which current tried to Recv or HasData from.
    const auto next = m_next_party_cand.back();

    // if this party has already finished, then current will never be able to
    // finish, so we crash the simulation here.
    if (HasTerminated(next)) {
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

  std::size_t next = Next(current.value(), m_nparties);
  std::size_t terminated = 0;
  while (terminated < m_nparties) {
    if (!HasTerminated(next)) {
      return next;
    }
    terminated++;
    next = Next(next, m_nparties);
  }

  return {};
}

util::Time::Duration sim::Context::Checkpoint(std::size_t id) {
  const auto latest = LatestTimestamp(id);
  const auto last_checkpoint = m_checkpoint;
  UpdateCheckpoint();
  return latest + (m_checkpoint - last_checkpoint);
}

void sim::Context::Prepare(std::size_t id) {
  if (m_state == State::COMMIT || m_state == State::ROLLBACK) {
    // Save the current head of m_traces so we can discard new events if this
    // party has to rollback.
    m_trace_index = m_traces[id].size();
    m_next_party_cand.clear();

    // Save the current m_writes map. Recv operations will change writes made by
    // other parties, so this is the easiest way to make sure Rollback does the
    // right thing.
    m_writes_backup = m_writes;
    for (std::size_t i = 0; i < m_nparties; ++i) {
      auto cid = ChannelId(id, i);
      m_buffers[cid]->Prepare();
    }
  } else {
    throw std::logic_error("cannot prepare ctx");
  }
  m_state = State::PREPARE;
}

void sim::Context::Commit(std::size_t id) {
  if (m_state == State::PREPARE) {
    m_writes_backup.clear();
    for (std::size_t i = 0; i < m_nparties; ++i) {
      ChannelId cid(id, i);
      m_buffers[cid]->Commit();
    }

  } else {
    throw std::logic_error("cannot commit");
  }
  m_state = State::COMMIT;
}

void sim::Context::Rollback(std::size_t id) {
  if (m_state == State::PREPARE) {
    m_traces[id].resize(m_trace_index);
    m_writes = m_writes_backup;
    for (std::size_t i = 0; i < m_nparties; ++i) {
      ChannelId cid(id, i);
      m_buffers[cid]->Rollback();
    }
  } else {
    throw std::logic_error("cannot rollback");
  }
  m_state = State::ROLLBACK;
}
