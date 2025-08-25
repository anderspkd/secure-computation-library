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

#ifndef SCL_UTIL_TIME_H
#define SCL_UTIL_TIME_H

#include <chrono>
#include <ratio>

namespace scl::util {

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
  using namespace std::chrono;
  return duration<long double, std::milli>(time).count();
}

}  // namespace scl::util

#endif  // SCL_UTIL_TIME_H
