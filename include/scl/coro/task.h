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

#ifndef SCL_CORO_TASK_H
#define SCL_CORO_TASK_H

#include <coroutine>
#include <ios>
#include <memory>
#include <type_traits>
#include <utility>

#include "scl/coro/batch.h"
#include "scl/coro/promise.h"

namespace scl::coro {

class Runtime;

namespace details {

/**
 * @brief Remove a handle from a runtime.
 * @param runtime the runtime.
 * @param handle handle for the coroutine.
 *
 * This function is used when Task destroys the coroutine state. Part of this
 * teardown involves telling the current runtime that the handle should not be
 * considered for resumption anymore.
 */
void removeHandle(Runtime* runtime, std::coroutine_handle<> handle);

}  // namespace details

/**
 * @brief A coroutine task.
 * @tparam RESULT the type of the return value of the coroutine.
 *
 * coro::Task specifies a coroutine which returns a value of type \p RESULT.
 * Tasks are cold start, i.e., they wont start executing until they are awaited.
 *
 * Tasks are move only types and considered the unique owner of the
 * std::coroutine_handle which is associated with the coroutine.
 */
template <typename RESULT = void>
class Task {
 public:
  /**
   * @brief Promise type of Task.
   */
  using promise_type = details::TaskPromise<RESULT>;

  /**
   * @brief Destructor.
   *
   * The destructor of Task will first tell the current runtime to stop tracking
   * the coroutine handle that this task manages. Afterwards the handle is
   * destroyed.
   */
  ~Task() {
    destroy();
  }

  /**
   * @brief Move constructor.
   */
  Task(Task&& other) noexcept
      : m_handle(std::exchange(other.m_handle, nullptr)) {}

  /**
   * @brief Construction from copy not allowed.
   */
  Task(const Task&) = delete;

  /**
   * @brief Move assignment.
   */
  Task& operator=(Task&& other) noexcept {
    destroy();
    m_handle = std::exchange(other.m_handle, nullptr);
    return *this;
  }

  /**
   * @brief Assignment from copy not allowed.
   */
  Task& operator=(const Task&) = delete;

  /**
   * @brief Allows a task to be co_await'ed.
   */
  auto operator co_await() {
    struct Awaiter {
      std::coroutine_handle<promise_type> coroutine;

      // the awaiting corouting can resume immediately if the task it is waiting
      // for already finished.
      bool await_ready() noexcept {
        return coroutine.done();
      }

      // the awaiting coroutine is suspended. So we will resume the task it is
      // waiting for, and designate the awaiter as the coroutine that should be
      // run when the task finishes.
      std::coroutine_handle<promise_type> await_suspend(
          std::coroutine_handle<> awaiter) {
        coroutine.promise().setNext(awaiter);
        return coroutine;
      }

      // get the result of the task being co_await'ed.
      auto await_resume() {
        return coroutine.promise().result();
      }
    };

    return Awaiter{m_handle};
  }

  /**
   * @brief Set the Runtime for executing this Task.
   */
  void setRuntime(Runtime* runtime) {
    m_handle.promise().setRuntime(runtime);
  }

  /**
   * @brief Destroy this task.
   */
  void destroy() {
    if (m_handle) {
      details::removeHandle(m_handle.promise().getRuntime(), m_handle);
      m_handle.destroy();
    }
  }

  /**
   * @brief Check if a result is ready.
   */
  bool ready() const {
    return m_handle.done();
  }

  /**
   * @brief Get the return value of this task.
   */
  RESULT result() const {
    return m_handle.promise().result();
  }

  /**
   * @brief The coroutine handle associated with this task.
   */
  std::coroutine_handle<promise_type> m_handle;

 private:
  friend promise_type;

  explicit Task(std::coroutine_handle<promise_type> handle)
      : m_handle(handle) {}
};

namespace details {

template <typename RESULT>
Task<RESULT> TaskPromise<RESULT>::get_return_object() {
  return Task<RESULT>(
      std::coroutine_handle<TaskPromise<RESULT>>::from_promise(*this));
}

inline Task<void> TaskPromise<void>::get_return_object() {
  return Task<void>(
      std::coroutine_handle<TaskPromise<void>>::from_promise(*this));
}

}  // namespace details
}  // namespace scl::coro

#endif  // SCL_CORO_TASK_H
