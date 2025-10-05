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

#include "scl/simulation/runtime.h"

#include <coroutine>

using namespace scl;

void details::SimulatorRuntime::schedule(std::coroutine_handle<> coroutine,
                                         std::function<bool()>&& predicate) {
  m_tq.emplace_back(coroutine, std::move(predicate), m_current_pid);
}

void details::SimulatorRuntime::schedule(std::coroutine_handle<> coroutine,
                                         Time::Duration delay) {
  auto ctx = m_sim_ctx.getContext(m_current_pid);
  const auto et = ctx.lastEvent()->time();
  ctx.addEvent<SleepEvent>(et + delay, delay);
  this->deschedule(coroutine);
}

void details::SimulatorRuntime::scheduleWithId(
    std::coroutine_handle<> coroutine,
    std::size_t pid) {
  m_tq.emplace_back(coroutine, []() { return true; }, pid);
}

void details::SimulatorRuntime::deschedule(std::coroutine_handle<> coroutine) {
  m_tq.remove_if(
      [&coroutine](const Coro& coro) { return coro.coroutine == coroutine; });
}

std::coroutine_handle<> details::SimulatorRuntime::next() {
  auto beg = m_tq.begin();
  const auto end = m_tq.end();

  while (beg != end) {
    const auto [coroutine, predicate, pid] = *beg;

    if (predicate()) {
      m_tq.erase(beg);
      m_current_pid = pid;

      if (pid != MANAGE_PID) {
        m_sim_ctx.getContext(m_current_pid).startClock();
      }

      return coroutine;
    }

    ++beg;
  }

  return std::noop_coroutine();
}
