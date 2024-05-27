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

#ifndef SCL_CORO_BATCH_H
#define SCL_CORO_BATCH_H

#include <algorithm>
#include <coroutine>
#include <optional>
#include <type_traits>
#include <vector>

namespace scl::coro {

template <typename RESULT>
class Task;

class Runtime;

namespace details {

/**
 * @brief A Batch of coroutine tasks.
 * @tparam RESULT the result type of the tasks.
 *
 * The primary task of this class is to enable manual scheduling of a list of
 * tasks. This allows the running the tasks that the batch is constructed with
 * to run in parallel.
 */
template <typename RESULT>
class Batch final {
 public:
  /**
   * @brief Construct a new batch.
   * @param tasks the tasks in this batch.
   */
  explicit Batch(std::vector<Task<RESULT>>&& tasks)
      : m_tasks(std::move(tasks)) {}

  /**
   * @brief Check if this batch is ready.
   * @return true if all tasks in the batch have finished, false otherwise.
   */
  bool await_ready() const noexcept {
    return !std::any_of(m_tasks.begin(), m_tasks.end(), [](const auto& task) {
      return !task.ready();
    });
  }

  /**
   * @brief Run when the coroutine waiting for this batch suspends.
   * @param coroutine the coroutine co_await'ing this batch.
   * @return coroutine to run instead.
   */
  std::coroutine_handle<> await_suspend(std::coroutine_handle<> coroutine);

  /**
   * @brief Run when the batch has finished.
   */
  template <typename R = RESULT>
  auto await_resume() -> std::enable_if_t<std::is_void_v<R>, void> {
    for (const auto& task : m_tasks) {
      task.result();
    }
  }

  /**
   * @brief Run when the batch has finished.
   */
  template <typename R = RESULT>
  auto await_resume() -> std::enable_if_t<!std::is_void_v<R>, std::vector<R>> {
    std::vector<RESULT> results;
    for (const auto& task : m_tasks) {
      results.emplace_back(task.result());
    }
    return results;
  }

  /**
   * @brief Assign the runtime to use for this batch.
   * @param runtime the runtime.
   *
   * The runtime passed here is set as the runtime for all Tasks in the batch.
   */
  void setRuntime(Runtime* runtime) noexcept {
    m_runtime = runtime;
  }

 private:
  std::vector<Task<RESULT>> m_tasks;
  Runtime* m_runtime = nullptr;
};

/**
 * @brief A batch of coroutine tasks.
 * @tparam RESULT the result type of the tasks.
 *
 * Similar to Batch, except with a weaker requirement on the number of tasks
 * that must complete. Tasks that do not complete are destroyed automatically
 * when the PartialBatch is destroyed.
 */
template <typename RESULT>
class PartialBatch final {
 public:
  /**
   * @brief Construct a new PartialBatch.
   * @param tasks the tasks in the batch.
   * @param min_complete the minimum number of tasks that should complete before
   * the batch is done.
   */
  explicit PartialBatch(std::vector<Task<RESULT>>&& tasks,
                        std::size_t min_complete)
      : m_tasks(std::move(tasks)), m_min_complete(min_complete) {}

  /**
   * @brief Check if the batch is done.
   * @return true if the number of ready tasks exceed min_complete.
   */
  bool await_ready() const noexcept {
    std::size_t count = 0;
    for (const auto& task : m_tasks) {
      if (task.ready()) {
        count++;
      }
    }
    return count >= m_min_complete;
  }

  /**
   * @brief Suspend this batch.
   */
  std::coroutine_handle<> await_suspend(std::coroutine_handle<> coroutine);

  /**
   * @brief Resume this batch.
   */
  template <typename R = RESULT>
  auto await_resume() -> std::enable_if_t<std::is_void_v<R>, void> {
    for (const auto& task : m_tasks) {
      if (task.ready()) {
        task.result();
      }
    }
  }

  /**
   * @brief Resume this batch
   */
  template <typename R = RESULT>
  auto await_resume()
      -> std::enable_if_t<!std::is_void_v<R>, std::vector<std::optional<R>>> {
    std::vector<std::optional<R>> results;
    for (const auto& task : m_tasks) {
      if (task.ready()) {
        results.emplace_back(task.result());
      } else {
        results.emplace_back(std::optional<R>{});
      }
    }
    return results;
  }

  /**
   * @brief Set the runtime to use for running this batch.
   * @param runtime the runtime.
   *
   * \p runtime is assigned as the runtime for all Tasks in this batch.
   */
  void setRuntime(Runtime* runtime) noexcept {
    m_runtime = runtime;
  }

 private:
  std::vector<Task<RESULT>> m_tasks;
  std::size_t m_min_complete;

  Runtime* m_runtime = nullptr;
};

}  // namespace details

/**
 * @brief Create a new batch task.
 * @param tasks the tasks in the batch.
 *
 * Creates a batch of coroutine tasks that will be run concurrently when
 * <code>co_await</code>'ed. This can therefore be used when, for example, we
 * would like one task to resume when another is suspended (e.g., because the
 * suspended task is waiting for data).
 *
 * @code
 * std::vector<Task<int>> tasks;
 * tasks.emplace_back(intTask());
 * tasks.emplace_back(anoterIntTask());
 *
 * std::vector<int> results = co_await batch(std::move(tasks));
 * @endcode
 */
template <typename RESULT>
auto batch(std::vector<Task<RESULT>>&& tasks) {
  return details::Batch{std::move(tasks)};
}

/**
 * @brief Create a new batch task.
 * @param tasks the tasks in the batch.
 * @param min_complete the minimum number of tasks to complete for the batch to
 * be considered complete.
 *
 * Creates a batch of coroutine tasks, where we are just interested in some of
 * the finishing.
 *
 * @code
 * std::vector<Task<int>> tasks;
 * tasks.emplace_back(intTask());
 * tasks.emplace_back(intTaskThatRunsForever());
 *
 * std::vector<std::optional<int>> results = co_await batch(std::move(tasks),
 * 1);
 *
 * results[0].has_value();  // returns true
 * results[1].has_value();  // returns false
 * @endcode
 */
template <typename RESULT>
auto batch(std::vector<RESULT>&& tasks, std::size_t min_complete) {
  return details::PartialBatch{std::move(tasks), min_complete};
}

}  // namespace scl::coro

#endif  // SCL_CORO_BATCH_H
