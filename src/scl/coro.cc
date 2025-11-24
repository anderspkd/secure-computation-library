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

#include "scl/coro.h"

#include <coroutine>

using namespace scl;

void details::removeHandle(Runtime* runtime, std::coroutine_handle<> handle) {
  if (runtime != nullptr) {
    runtime->deschedule(handle);
  }
}

std::coroutine_handle<> DefaultRuntime::next() {
  auto b = m_tq.begin();
  const auto e = m_tq.end();
  while (b != e) {
    const auto [coro, pred] = *b;
    if (pred()) {
      m_tq.erase(b);
      return coro;
    }
    b++;
  }
  return std::noop_coroutine();
}

void DefaultRuntime::deschedule(std::coroutine_handle<> coroutine) {
  m_tq.remove_if([&coroutine](const Pair& pair) {
    return std::get<0>(pair) == coroutine;
  });
}

std::coroutine_handle<> details::SleepAwaiter::await_suspend(
    std::coroutine_handle<> handle) {
  m_runtime->schedule(handle, m_duration);
  return m_runtime->next();
}
