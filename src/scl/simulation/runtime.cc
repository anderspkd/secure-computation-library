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

#include "scl/simulation/runtime.h"

#include <coroutine>

#include "scl/simulation/context.h"

using namespace scl;

void sim::details::SimulatorRuntime::schedule(
    std::coroutine_handle<> coroutine,
    std::function<bool()>&& predicate) {
  m_tq.emplace_back(coroutine, std::move(predicate), m_current_pid);
}

void sim::details::SimulatorRuntime::schedule(std::coroutine_handle<> coroutine,
                                              util::Time::Duration delay) {
  auto view = m_ctx.view(m_current_pid);
  const auto last = view.lastEventTimestamp();
  view.recordEvent(Event::sleep(last, delay));
  this->schedule(coroutine);
}

void sim::details::SimulatorRuntime::deschedule(
    std::coroutine_handle<> coroutine) {
  m_tq.remove_if(
      [&coroutine](const Coro& coro) { return coro.coroutine == coroutine; });
}

// void sim::details::SimulatorRuntime::removeCancelledCoros() {
//   auto b = m_tq.begin();
//   const auto e = m_tq.end();

//   while (b != e) {
//     const auto& [coro, pred, pid] = *b;
//     if (pid != MANAGER_PID && m_ctx.cancellation_map.at(pid)) {

//     }
//   }
// }

std::coroutine_handle<> sim::details::SimulatorRuntime::next() {
  auto b = m_tq.begin();
  const auto e = m_tq.end();

  while (b != e) {
    const auto [coro, pred, pid] = *b;

    // if we're about to run the manager coro, then we do not wish
    // to check if it's been cancelled.
    if (pid != MANAGER_PID && m_ctx.cancellation_map.at(pid)) {
    } else if (pred()) {
      m_tq.erase(b);
      m_current_pid = pid;

      // Event timestamps are computed as
      //
      //  E[i].ts = E[i - 1] + (now - last_startClock)
      //
      // It is therefore important that startClock is called here, since
      // otherwise we may end up counting time spent executing another party (or
      // just time spent in the simulation runtime), when we compute the
      // timestamp of event i.
      if (m_current_pid != MANAGER_PID) {
        m_ctx.view(m_current_pid).startClock();
      }
      return coro;
    }
    b++;
  }

  return std::noop_coroutine();
}
