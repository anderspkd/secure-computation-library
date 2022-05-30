/**
 * @file shared_deque.h
 *
 * SCL --- Secure Computation Library
 * Copyright (C) 2022 Anders Dalskov
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

#ifndef _SCL_NET_SHARED_DEQUE_H
#define _SCL_NET_SHARED_DEQUE_H

#include <condition_variable>
#include <mutex>
#include <queue>

namespace scl {
namespace details {

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
  T &Peek();

  /**
   * @brief Remove and return the top element from the queue.
   */
  T Pop();

  /**
   * @brief Insert an item to the back of the queue.
   */
  void PushBack(const T &item);

  /**
   * @brief Move an item to the back of the queue.
   */
  void PushBack(T &&item);

  /**
   * @brief Number of elements currently in the queue.
   */
  std::size_t Size();

 private:
  /**
   * @brief Underlying STL deque.
   */
  std::deque<T, Allocator> mDeck;

  /**
   * @brief mutex.
   */
  std::mutex mMutex;

  /**
   * @brief condition variable.
   */
  std::condition_variable mCond;
};

template <typename T, typename Allocator>
void SharedDeque<T, Allocator>::PopFront() {
  std::unique_lock<std::mutex> lock(mMutex);
  while (mDeck.empty()) {
    mCond.wait(lock);
  }
  mDeck.pop_front();
}

template <typename T, typename Allocator>
T &SharedDeque<T, Allocator>::Peek() {
  std::unique_lock<std::mutex> lock(mMutex);
  while (mDeck.empty()) {
    mCond.wait(lock);
  }
  return mDeck.front();
}

template <typename T, typename Allocator>
T SharedDeque<T, Allocator>::Pop() {
  std::unique_lock<std::mutex> lock(mMutex);
  while (mDeck.empty()) {
    mCond.wait(lock);
  }
  auto x = mDeck.front();
  mDeck.pop_front();
  return x;
}

template <typename T, typename Allocator>
void SharedDeque<T, Allocator>::PushBack(const T &item) {
  std::unique_lock<std::mutex> lock(mMutex);
  mDeck.push_back(item);
  lock.unlock();
  mCond.notify_one();
}

template <typename T, typename Allocator>
void SharedDeque<T, Allocator>::PushBack(T &&item) {
  std::unique_lock<std::mutex> lock(mMutex);
  mDeck.push_back(std::move(item));
  lock.unlock();
  mCond.notify_one();
}

template <typename T, typename Allocator>
std::size_t SharedDeque<T, Allocator>::Size() {
  std::unique_lock<std::mutex> lock(mMutex);
  auto size = mDeck.size();
  lock.unlock();
  return size;
}

}  // namespace details
}  // namespace scl

#endif  // _SCL_NET_SHARED_DEQUE_H
