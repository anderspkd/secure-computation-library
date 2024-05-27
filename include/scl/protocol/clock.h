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

#ifndef SCL_PROTOCOL_CLOCK_H
#define SCL_PROTOCOL_CLOCK_H

#include "scl/util/time.h"

namespace scl::proto {

/**
 * @brief A clock interface.
 */
struct Clock {
  virtual ~Clock() {}

  /**
   * @brief Read the current value of the clock.
   */
  virtual util::Time::Duration read() const = 0;
};

/**
 * @brief A clock implementation based on real time.
 */
class RealtimeClock final : public Clock {
 public:
  /**
   * @brief Create a new RealtimeClock.
   */
  RealtimeClock() : m_clock_start(util::Time::now()) {}

  /**
   * @brief Read the current value of the clock.
   * @return the amount of time elapsed since this clock was created.
   */
  util::Time::Duration read() const override {
    return util::Time::now() - m_clock_start;
  }

 private:
  util::Time::TimePoint m_clock_start;
};

}  // namespace scl::proto

#endif  // SCL_PROTOCOL_CLOCK_H
