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
#include <ratio>

namespace scl {

/**
 * @brief Wrapper for time related types.
 */
struct Time {
  /**
   * @brief Duration type.
   */
  using Duration = std::chrono::steady_clock::duration;

  /**
   * @brief Time point type.
   */
  using TimePoint = std::chrono::steady_clock::time_point;

  /**
   * @brief Get the current time as a TimePoint.
   */
  static TimePoint now() {
    return std::chrono::steady_clock::now();
  };
};

/**
 * @brief Convert a timestamp to milliseconds.
 */
inline long double timeToMillis(Time::Duration time) {
  return std::chrono::duration<long double, std::milli>(time).count();
}

/**
 * @brief Clock interface.
 *
 * Clock is used within protocols to get the time elapsed since the protocol was
 * first started. The reason for requiring an interface to get this information
 * is because protocols might be run in a simulation, in which case wall-clock
 * time wouldn't be accurate.
 */
struct Clock {
  virtual ~Clock() {}

  /**
   * @brief Read the value of the clock.
   */
  virtual Time::Duration read() const = 0;
};

/**
 * @brief A Clock implementation based on real time.
 */
class RealtimeClock final : public Clock {
 public:
  RealtimeClock() : m_start(Time::now()) {}

  Time::Duration read() const override {
    return Time::now() - m_start;
  }

 private:
  Time::TimePoint m_start;
};

}  // namespace scl
