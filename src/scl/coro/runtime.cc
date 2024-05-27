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

#include "scl/coro/runtime.h"

#include <coroutine>

#include "scl/coro/task.h"

using namespace scl;

void coro::details::removeHandle(Runtime* runtime,
                                 std::coroutine_handle<> handle) {
  if (runtime != nullptr) {
    runtime->deschedule(handle);
  }
}

std::coroutine_handle<> coro::DefaultRuntime::next() {
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

void coro::DefaultRuntime::deschedule(std::coroutine_handle<> coroutine) {
  m_tq.remove_if([&coroutine](const Pair& pair) {
    return std::get<0>(pair) == coroutine;
  });
}
