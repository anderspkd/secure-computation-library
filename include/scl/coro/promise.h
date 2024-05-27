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

#ifndef SCL_CORO_PROMISE_H
#define SCL_CORO_PROMISE_H

#include <chrono>
#include <coroutine>
#include <exception>
#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>

#include "scl/coro/batch.h"
#include "scl/coro/future.h"
#include "scl/coro/sleep_awaiter.h"
#include "scl/util/time.h"

namespace scl::coro {

template <typename RESULT>
class Task;

class Runtime;

namespace details {

/**
 * @brief Base type for the promise_type of Tasks.
 */
class TaskPromiseBase {
  /**
   * @brief Awaiter returned on final_suspend.
   *
   * The job of this awaiter is to suspend execution to prevent the coroutine
   * result from simply dissapearing, and to resume any tasks that are waiting
   * for this coroutine to finish.
   */
  struct FinalAwaiter {
    bool await_ready() noexcept {
      return false;
    }

    template <typename PROMISE>
    std::coroutine_handle<> await_suspend(
        std::coroutine_handle<PROMISE> handle) noexcept {
      if (handle.promise().m_next) {
        return handle.promise().m_next;
      }
      return std::noop_coroutine();
    }

    void await_resume() noexcept {}
  };

 public:
  /**
   * @brief Cold-starts the coroutine.
   */
  auto initial_suspend() noexcept {
    return std::suspend_always{};
  }

  /**
   * @brief Called when the Task finishes running.
   */
  auto final_suspend() noexcept {
    return FinalAwaiter{};
  }

  /**
   * @brief Transform an awaitable into an awaiter.
   * @param awaitable the awaitable.
   *
   * The assumption made here is that the awaitable is also the awaiter. In
   * particular, the type has the required functions for it to be an awaiter. In
   * addition, the type should possess a <code>setRuntime(coro::Runtime*)</code>
   * function which is used to specify which runtime should be used when
   * suspending the awaitable.
   */
  template <typename AWAITABLE>
  AWAITABLE await_transform(AWAITABLE&& awaitable);

  /**
   * @brief Transform called on future types.
   *
   * A "future" is any callable which returns either true or false. The callable
   * describes when the waiting coroutine can be resumed.
   */
  template <FutureAwaitableType FUTURE>
  auto await_transform(FUTURE&& future) {
    return await_transform(FutureAwaiter<FUTURE>(std::forward<FUTURE>(future)));
  }

  /**
   * @brief Transform called on a std::chrono::duration.
   */
  template <typename REP, typename PERIOD>
  auto await_transform(std::chrono::duration<REP, PERIOD> duration) {
    return await_transform(details::SleepAwaiter(duration));
  }

  /**
   * @brief Set the coroutine to run when this task finishes.
   */
  void setNext(std::coroutine_handle<> next) {
    m_next = next;
  }

  /**
   * @brief Set the Runtime to use for executing this task.
   */
  void setRuntime(Runtime* runtime) {
    m_runtime = runtime;
  }

  /**
   * @brief Get the current Runtime.
   */
  Runtime* getRuntime() const {
    return m_runtime;
  }

 private:
  std::coroutine_handle<> m_next;
  Runtime* m_runtime = nullptr;
};

/**
 * @brief Task promise type for general non-void return types.
 */
template <typename RESULT>
class TaskPromise final : public TaskPromiseBase {
 public:
  /**
   * @brief Create a Task object from this promise.
   */
  Task<RESULT> get_return_object();

  /**
   * @brief Set the return value of this Task.
   * @param result the value to return when this Task completes.
   */
  void return_value(RESULT result) {
    m_result.template emplace<1>(std::move(result));
  }

  /**
   * @brief Called if the Task throws an exception.
   */
  void unhandled_exception() noexcept {
    m_result.template emplace<2>(std::current_exception());
  }

  /**
   * @brief Get the return value of this Task.
   */
  RESULT result() {
    if (m_result.index() == 0) {
      throw std::logic_error("result() called on unfinished coroutine");
    }
    if (m_result.index() == 2) {
      std::rethrow_exception(std::get<2>(m_result));
    }
    return std::get<1>(std::move(m_result));
  }

 private:
  std::variant<std::monostate, RESULT, std::exception_ptr> m_result;
};

/**
 * @brief Task promise specialization for Tasks returning void.
 */
template <>
class TaskPromise<void> final : public TaskPromiseBase {
 public:
  /**
   * @brief Create a Task object from this promise.
   */
  Task<void> get_return_object();

  /**
   * @brief Indicates that the Task has finished executing.
   */
  void return_void() noexcept {}

  /**
   * @brief Called if the Task throws an exception.
   */
  void unhandled_exception() noexcept {
    m_exception = std::current_exception();
  }

  /**
   * @brief Get the result of this Task.
   *
   * Calling this function will rethrow any exception that the Task throw while
   * executing. If the Task executed without error, then this function does
   * nothing.
   */
  void result() {
    if (m_exception) {
      std::rethrow_exception(m_exception);
    }
  }

 private:
  std::exception_ptr m_exception = nullptr;
};

}  // namespace details
}  // namespace scl::coro

#endif  // SCL_CORO_PROMISE_H
