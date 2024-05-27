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

#ifndef SCL_CORO_FUTURE_H
#define SCL_CORO_FUTURE_H

#include <concepts>
#include <coroutine>
#include <functional>
#include <memory>

namespace scl::coro {

class Runtime;

namespace details {

/**
 * @brief Concept that a future type must satisfy.
 */
template <typename FUTURE>
concept FutureAwaitableType = requires(FUTURE future) {
                                { future() } -> std::convertible_to<bool>;
                              };

/**
 * @brief The awaiter for future events.
 * @tparam FUTURE_EVENT the type of the future event.
 *
 * \p FUTURE_EVENT must be a subclass of FutureEvent.
 */
template <FutureAwaitableType FUTURE>
class FutureAwaiter final {
 public:
  /**
   * @brief Construct a new awaiter from a future.
   * @param future the future.
   */
  FutureAwaiter(FUTURE&& future) : m_future(std::forward<FUTURE>(future)){};

  /**
   * @brief Futures are by design not ready immediately.
   */
  bool await_ready() const noexcept {
    return false;
  }

  /**
   * @brief Schedule the coroutine for later execution, pending some condition.
   * @return the next coroutine to execute.
   */
  std::coroutine_handle<> await_suspend(std::coroutine_handle<> handle);

  /**
   * @brief Does nothing.
   */
  void await_resume() const noexcept {}

  /**
   * @brief Sets the runtime for this future.
   */
  void setRuntime(Runtime* runtime) {
    m_runtime = runtime;
  }

 private:
  Runtime* m_runtime;
  FUTURE m_future;
};

}  // namespace details
}  // namespace scl::coro

#endif  // SCL_CORO_FUTURE_H
