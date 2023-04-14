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

#ifndef SCL_SIMULATION_ENV_H
#define SCL_SIMULATION_ENV_H

#include <chrono>

#include "scl/protocol/env.h"
#include "scl/simulation/context.h"
#include "scl/simulation/event.h"

namespace scl::sim {

/**
 * @brief A ProtocolEnvironment::Clock implementation for simulated protocols.
 */
class SimulatedClock final : public proto::ProtocolEnvironment::Clock {
 public:
  /**
   * @brief Create a new clock for simulations.
   * @param ctx a simulation context. Used to read the current time of the party
   * @param id the ID of the party
   */
  SimulatedClock(std::shared_ptr<SimulationContext> ctx, std::size_t id)
      : m_ctx(ctx), m_id(id){};

  /**
   * @brief Get the total elapsed time of this party.
   *
   * This method will compute a running time based on the current checkpoint in
   * the simulation context, and offset this with the timestamp from the last
   * event that the party generated.
   */
  util::Time::Duration Read() const override {
    const auto now = util::Time::Now();
    const auto ts = m_ctx->LatestTimestamp(m_id);
    return now - m_ctx->ReadCurrentCheckpoint() + ts;
  }

  /**
   * @brief Mark a checkpoint.
   */
  void Checkpoint(const std::string& message) override {
    m_ctx->AddEvent(m_id, std::make_shared<CheckpointEvent>(Read(), message));
  }

 private:
  std::shared_ptr<SimulationContext> m_ctx;
  std::size_t m_id;
};

/**
 * @brief A ProtocolEnvironment::Thread implementation for simulated protocols.
 */
class SimulatedThreadCtx final : public proto::ProtocolEnvironment::Thread {
 public:
  /**
   * @brief Create a new thread context for simulations.
   * @param ctx a simulation context
   * @param id the ID of the party
   */
  SimulatedThreadCtx(std::shared_ptr<SimulationContext> ctx, std::size_t id)
      : m_ctx(ctx), m_id(id){};

  /**
   * @brief Simulate a sleep for this party.
   * @param ms the time to sleep this party in milliseconds
   *
   * The main thread of this party is put to sleep by generating a
   * <code>SLEEP</code> event with the time <code>t0 + ms</code>, where
   * <code>t0</code> is the current time of the party.
   */
  void Sleep(std::size_t ms) override {
    const auto now = m_ctx->Checkpoint(m_id);
    const auto event = std::make_shared<Event>(Event::Type::SLEEP,
                                               now,
                                               std::chrono::milliseconds(ms));
    m_ctx->AddEvent(m_id, event);
  }

 private:
  std::shared_ptr<SimulationContext> m_ctx;
  std::size_t m_id;
};

}  // namespace scl::sim

#endif  // SCL_SIMULATION_ENV_H
