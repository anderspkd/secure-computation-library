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

#pragma once

#include <chrono>
#include <coroutine>
#include <exception>
#include <functional>
#include <list>
#include <utility>
#include <variant>

#include "scl/time.h"

namespace scl {

template <typename RESULT>
class Task;

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
  SleepAwaiter(Time::Duration duration) : m_duration(duration) {}

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
  Time::Duration m_duration;

  Runtime* m_runtime = nullptr;
};

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
  FutureAwaiter(FUTURE&& future) : m_future(std::forward<FUTURE>(future)) {};

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
                        Time::Duration delay) = 0;

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
                Time::Duration delay) override {
    const auto start = Time::now();
    schedule(coroutine, [start, delay]() {
      const auto now = Time::now();
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

}  // namespace details
}  // namespace scl
