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

#ifndef SCL_SIMULATION_RUNTIME_H
#define SCL_SIMULATION_RUNTIME_H

#include <list>
#include <memory>

#include "scl/coro/runtime.h"
#include "scl/simulation/context.h"
#include "scl/simulation/event.h"

namespace scl::sim::details {

/**
 * @brief Runtime implementation used in the simulator.
 */
class SimulatorRuntime final : public coro::Runtime {
  constexpr static std::size_t MANAGER_PID = -1;

  struct Coro {
    std::coroutine_handle<> coroutine;
    std::function<bool()> predicate;
    std::size_t party_id;
  };

 public:
  /**
   * @brief Construct a new simulator runtime.
   */
  SimulatorRuntime(GlobalContext& ctx)
      : m_ctx(ctx), m_current_pid(MANAGER_PID) {}

  ~SimulatorRuntime() {}

  using coro::Runtime::schedule;

  /**
   * @brief Schedule a coroutine to run for a particular party.
   * @param coroutine the coroutine.
   * @param id the id of the party.
   *
   * This function is used when scheduling the initial batch of protocols. Each
   * protocol run gets assigned a party id using this function, and the ID is
   * then used throughout the execution in order correctly manipulate the
   * context.
   */
  void scheduleWithId(std::coroutine_handle<> coroutine, std::size_t id) {
    m_tq.emplace_back(
        coroutine,
        []() { return true; },
        id);
  }

  void schedule(std::coroutine_handle<> coroutine,
                std::function<bool()>&& predicate) override;

  void schedule(std::coroutine_handle<> coroutine,
                util::Time::Duration delay) override;

  void deschedule(std::coroutine_handle<> coroutine) override;

  std::coroutine_handle<> next() override;

  bool taskQueueEmpty() const override {
    return m_tq.empty();
  }

 private:
  GlobalContext& m_ctx;

  std::size_t m_current_pid;
  std::list<Coro> m_tq;

  void removeCancelledCoros();
};

}  // namespace scl::sim::details

#endif  // SCL_SIMULATION_RUNTIME_H
