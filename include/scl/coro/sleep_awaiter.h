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

#ifndef SCL_CORO_SLEEP_AWAITER_H
#define SCL_CORO_SLEEP_AWAITER_H

#include <coroutine>
#include <functional>

#include "scl/util/time.h"

namespace scl::coro {

class Runtime;

namespace details {

/**
 * @brief Awaiter interface for suspending coroutines for some amount of time.
 */
class SleepAwaiter {
 public:
  /**
   * @brief Create a new sleep awaiter.
   */
  SleepAwaiter(util::Time::Duration duration) : m_duration(duration) {}

  /**
   * @brief Check if the sleep awaiter is ready.
   *
   * The assumption made is that <code>duration > 0</code>, and so this function
   * always returns false.
   */
  bool await_ready() const noexcept {
    return false;
  }

  /**
   * @brief Suspend the coroutine that is being put to sleep.
   */
  std::coroutine_handle<> await_suspend(std::coroutine_handle<> handle);

  /**
   * @brief Resume the coroutine. Does nothing.
   */
  void await_resume() const noexcept {};

  /**
   * @brief Set the runtime for this coroutine.
   */
  void setRuntime(Runtime* runtime) {
    m_runtime = runtime;
  }

 private:
  util::Time::Duration m_duration;

  Runtime* m_runtime = nullptr;
};

}  // namespace details
}  // namespace scl::coro

#endif  // SCL_CORO_SLEEP_AWAITER_H
