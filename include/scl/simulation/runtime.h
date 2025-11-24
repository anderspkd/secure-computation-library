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

#pragma once

#include <coroutine>

#include "scl/coro.h"
#include "scl/simulation/context.h"

namespace scl::details {

/**
 * @brief Coroutine runtime used in simulations.
 * @ingroup eval-sim
 */
class SimulatorRuntime final : public Runtime {
 private:
  static const std::size_t MANAGE_PID = static_cast<std::size_t>(-1);

 public:
  using Runtime::schedule;

  SimulatorRuntime(SimulatorContext& sim_ctx)
      : m_sim_ctx(sim_ctx), m_current_pid(MANAGE_PID) {}

  void schedule(std::coroutine_handle<> coroutine,
                std::function<bool()>&& predicate) override;

  void schedule(std::coroutine_handle<> coroutine,
                Time::Duration delay) override;

  void scheduleWithId(std::coroutine_handle<> coroutine, std::size_t pid);

  void deschedule(std::coroutine_handle<> coroutine) override;

  bool taskQueueEmpty() const override {
    return m_tq.empty();
  }

  std::coroutine_handle<> next() override;

 private:
  struct Coro {
    std::coroutine_handle<> coroutine;
    std::function<bool()> predicate;
    std::size_t pid;
  };

  SimulatorContext& m_sim_ctx;
  std::size_t m_current_pid;
  std::list<Coro> m_tq;
};

}  // namespace scl::details
