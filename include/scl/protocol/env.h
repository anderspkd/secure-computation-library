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

#ifndef SCL_PROTOCOL_ENV_H
#define SCL_PROTOCOL_ENV_H

#include <chrono>
#include <memory>
#include <ratio>
#include <thread>

#include "scl/net/network.h"
#include "scl/util/time.h"

namespace scl::proto {

/**
 * @brief Environment for protocol executions.
 *
 * ProtocolEnvironment is meant to provide a protocol controlled access to a
 * number of useful resources, such as a network, but also the ability to know
 * its total running time, as well as the ability to work with threads.
 */
struct ProtocolEnvironment {
  /**
   * @brief Interface for the environment's threading context.
   *
   * At the moment, the only threading related functionality that the
   * environment supports is the ability to sleep the current thread for a
   * provided amount of time.
   */
  struct Thread {
    virtual ~Thread(){};

    /**
     * @brief Put this thread to sleep.
     * @param ms the time to sleep this thread for, in milliseconds
     */
    virtual void Sleep(std::size_t ms) = 0;
  };

  /**
   * @brief Interface for the environment's clock context.
   */
  struct Clock {
    virtual ~Clock(){};

    /**
     * @brief Read the current value of the clock.
     */
    virtual util::Time::Duration Read() const = 0;
  };

  /**
   * @brief The network.
   */
  net::Network network;

  /**
   * @brief Clock.
   */
  std::unique_ptr<Clock> clock;

  /**
   * @brief Threading context.
   */
  std::unique_ptr<Thread> thread_ctx;
};

/**
 * @brief A protocol clock which returns the total running time.
 */
class RealTimeClock final : public ProtocolEnvironment::Clock {
 public:
  RealTimeClock() : m_init_time(util::Time::Now()){};
  ~RealTimeClock(){};

  util::Time::Duration Read() const {
    return util::Time::Now() - m_init_time;
  };

 private:
  util::Time::TimePoint m_init_time;
};

/**
 * @brief A protocol thread context which uses STL thread.
 */
class StlThreadContext final : public ProtocolEnvironment::Thread {
 public:
  ~StlThreadContext(){};

  /**
   * @brief Sleep the current thread using std::this_thread::sleep_for.
   */
  void Sleep(std::size_t ms) override {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
  };
};

}  // namespace scl::proto

#endif  // SCL_PROTOCOL_ENV_H
