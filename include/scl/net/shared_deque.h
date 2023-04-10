/* SCL --- Secure Computation Library
 * Copyright (C) 2023 Anders Dalskov
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

#ifndef SCL_NET_SHARED_DEQUE_H
#define SCL_NET_SHARED_DEQUE_H

#include <condition_variable>
#include <mutex>
#include <queue>

namespace scl::net {

/**
 * @brief A simple thread safe double-ended queue.
 *
 * Based on https://codereview.stackexchange.com/q/238347
 */
template <typename T, typename Allocator = std::allocator<T>>
class SharedDeque {
 public:
  /**
   * @brief Remove the top element from the queue.
   */
  void PopFront();

  /**
   * @brief Read the top element from the queue.
   */
  T& Peek();

  /**
   * @brief Remove and return the top element from the queue.
   */
  T Pop();

  /**
   * @brief Insert an item to the back of the queue.
   */
  void PushBack(const T& item);

  /**
   * @brief Move an item to the back of the queue.
   */
  void PushBack(T&& item);

  /**
   * @brief Number of elements currently in the queue.
   */
  std::size_t Size();

 private:
  std::deque<T, Allocator> m_deck;
  std::mutex m_mutex;
  std::condition_variable m_cond;
};

template <typename T, typename Allocator>
void SharedDeque<T, Allocator>::PopFront() {
  std::unique_lock<std::mutex> lock(m_mutex);
  while (m_deck.empty()) {
    m_cond.wait(lock);
  }
  m_deck.pop_front();
}

template <typename T, typename Allocator>
T& SharedDeque<T, Allocator>::Peek() {
  std::unique_lock<std::mutex> lock(m_mutex);
  while (m_deck.empty()) {
    m_cond.wait(lock);
  }
  return m_deck.front();
}

template <typename T, typename Allocator>
T SharedDeque<T, Allocator>::Pop() {
  std::unique_lock<std::mutex> lock(m_mutex);
  while (m_deck.empty()) {
    m_cond.wait(lock);
  }
  auto x = m_deck.front();
  m_deck.pop_front();
  return x;
}

template <typename T, typename Allocator>
void SharedDeque<T, Allocator>::PushBack(const T& item) {
  std::unique_lock<std::mutex> lock(m_mutex);
  m_deck.push_back(item);
  lock.unlock();
  m_cond.notify_one();
}

template <typename T, typename Allocator>
void SharedDeque<T, Allocator>::PushBack(T&& item) {
  std::unique_lock<std::mutex> lock(m_mutex);
  m_deck.push_back(std::move(item));
  lock.unlock();
  m_cond.notify_one();
}

template <typename T, typename Allocator>
std::size_t SharedDeque<T, Allocator>::Size() {
  std::unique_lock<std::mutex> lock(m_mutex);
  auto size = m_deck.size();
  lock.unlock();
  return size;
}

}  // namespace scl::net

#endif  // SCL_NET_SHARED_DEQUE_H
