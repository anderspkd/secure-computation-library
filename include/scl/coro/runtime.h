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

#ifndef SCL_CORO_RUNTIME_H
#define SCL_CORO_RUNTIME_H

#include <chrono>
#include <coroutine>
#include <functional>
#include <iostream>
#include <list>

#include "scl/coro/batch.h"
#include "scl/coro/future.h"
#include "scl/coro/promise.h"
#include "scl/coro/sleep_awaiter.h"
#include "scl/coro/task.h"
#include "scl/util/time.h"

namespace scl::coro {

/**
 * @brief Interface for a coroutine runtime.
 *
 * <p>A coroutine runtime should be able to handle scheduling and descheduling
 * of coroutines, as well as determination of which coroutines gets to run next.
 *
 * <p>Scheduled coroutines can roughly be divided into three categories:
 * Coroutines that can be executed as soon as possible; coroutines which can be
 * executed when some predicate becomes true, and coroutines that can be
 * executed after some time has passed. This interface contains functions for
 * each case, and instantiations handle each slightly differently.
 */
class Runtime {
 public:
  virtual ~Runtime() {}

  /**
   * @brief Schedule a coroutine for execution when some predicate is true.
   * @param coroutine the coroutine.
   * @param predicate a predicate indicating when the coroutine can be resumed.
   */
  virtual void schedule(std::coroutine_handle<> coroutine,
                        std::function<bool()>&& predicate) = 0;

  /**
   * @brief Schedule a coroutine for execution after some delay.
   * @param coroutine the coroutine.
   * @param delay a delay.
   */
  virtual void schedule(std::coroutine_handle<> coroutine,
                        util::Time::Duration delay) = 0;

  /**
   * @brief Schedule a coroutine for execution immediately.
   * @param coroutine the coroutine.
   */
  void schedule(std::coroutine_handle<> coroutine) {
    return this->schedule(coroutine, []() { return true; });
  }

  /**
   * @brief Deschedule a coroutine.
   * @param coroutine the coroutine.
   */
  virtual void deschedule(std::coroutine_handle<> coroutine) = 0;

  /**
   * @brief Check if there are coroutines that still need to be executed.
   */
  virtual bool taskQueueEmpty() const = 0;

  /**
   * @brief Get the next coroutine to execute.
   */
  virtual std::coroutine_handle<> next() = 0;

  /**
   * @brief Assigns this runtime to an awaitable.
   * @param awaitable the awaitable, a type with a "setRuntime" function.
   */
  template <typename AWAITABLE>
  void assignTo(AWAITABLE& awaitable) {
    awaitable.setRuntime(this);
  }

  /**
   * @brief Run a task to completion.
   * @param task the task.
   * @return the result of running \p task.
   */
  template <typename RESULT>
  RESULT run(Task<RESULT>&& task) {
    task.setRuntime(this);
    schedule(task.m_handle);

    while (!taskQueueEmpty()) {
      next().resume();
    }

    if constexpr (std::is_void_v<RESULT>) {
      task.result();
    } else {
      return task.result();
    }
  }
};

/**
 * @brief A Default implementation for a coroutine runtime.
 */
class DefaultRuntime final : public Runtime {
  using Pair = std::pair<std::coroutine_handle<>, std::function<bool()>>;

 public:
  /**
   * @brief Create a default runtime.
   */
  static std::unique_ptr<Runtime> create() {
    return std::make_unique<DefaultRuntime>();
  }

  ~DefaultRuntime() {}

  void schedule(std::coroutine_handle<> coroutine,
                std::function<bool()>&& predicate) override {
    m_tq.emplace_back(coroutine, std::move(predicate));
  }

  void schedule(std::coroutine_handle<> coroutine,
                util::Time::Duration delay) override {
    const auto start = util::Time::now();
    schedule(coroutine, [start, delay]() {
      const auto now = util::Time::now();
      return now - start >= delay;
    });
  }

  void deschedule(std::coroutine_handle<> coroutine) override;

  bool taskQueueEmpty() const override {
    return m_tq.empty();
  }

  std::coroutine_handle<> next() override;

 private:
  std::list<Pair> m_tq;
};

namespace details {

template <typename AWAITABLE>
AWAITABLE TaskPromiseBase::await_transform(AWAITABLE&& awaitable) {
  m_runtime->assignTo(awaitable);
  return std::forward<AWAITABLE>(awaitable);
}

template <FutureAwaitableType FUTURE>
std::coroutine_handle<> FutureAwaiter<FUTURE>::await_suspend(
    std::coroutine_handle<> handle) {
  m_runtime->schedule(handle, std::move(m_future));
  return m_runtime->next();
}

template <typename RESULT>
std::coroutine_handle<> Batch<RESULT>::await_suspend(
    std::coroutine_handle<> coroutine) {
  for (auto& task : m_tasks) {
    task.setRuntime(m_runtime);
    m_runtime->schedule(task.m_handle);
  }

  m_runtime->schedule(coroutine, [this]() { return await_ready(); });

  return m_runtime->next();
}

template <typename RESULT>
std::coroutine_handle<> PartialBatch<RESULT>::await_suspend(
    std::coroutine_handle<> coroutine) {
  for (auto& task : m_tasks) {
    task.setRuntime(m_runtime);
    m_runtime->schedule(task.m_handle);
  }

  m_runtime->schedule(coroutine, [this]() { return await_ready(); });

  return m_runtime->next();
}

inline std::coroutine_handle<> SleepAwaiter::await_suspend(
    std::coroutine_handle<> handle) {
  m_runtime->schedule(handle, m_duration);
  return m_runtime->next();
}

}  // namespace details
}  // namespace scl::coro

#endif  // SCL_CORO_RUNTIME_H
