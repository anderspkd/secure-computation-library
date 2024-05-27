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

#include "scl/util/measurement.h"

#include <cmath>

#include "scl/util/time.h"

using namespace scl;

template <>
long double util::Measurement<long double>::zero() const {
  return 0;
}

template <>
util::Time::Duration util::Measurement<util::Time::Duration>::zero() const {
  return util::Time::Duration::zero();
}

template <>
long double util::Measurement<long double>::square(long double v) const {
  return v * v;
}

template <>
util::Time::Duration util::Measurement<util::Time::Duration>::square(
    util::Time::Duration dur) const {
  long double u = dur.count();
  std::chrono::duration<long double, util::Time::Duration::period> w(u * u);
  return std::chrono::duration_cast<util::Time::Duration>(w);
}

template <>
long double util::Measurement<long double>::sqrt(long double v) const {
  return std::sqrt(v);
}

template <>
util::Time::Duration util::Measurement<util::Time::Duration>::sqrt(
    util::Time::Duration dur) const {
  long double u = std::sqrt(dur.count());
  std::chrono::duration<long double, util::Time::Duration::period> w(u);
  return std::chrono::duration_cast<util::Time::Duration>(w);
}

std::ostream& util::operator<<(std::ostream& os,
                               const util::TimeMeasurement& measurement) {
  os << "{"
     << "\"mean\": " << util::timeToMillis(measurement.mean()) << ", "
     << "\"unit\": \"ms\", "
     << "\"std_dev\": " << util::timeToMillis(measurement.stddev()) << "}";

  return os;
}

std::ostream& util::operator<<(std::ostream& os,
                               const util::DataMeasurement& measurement) {
  os << "{"
     << "\"mean\": " << measurement.mean() << ", "
     << "\"unit\": \"B\", "
     << "\"std_dev\": " << measurement.stddev() << "}";

  return os;
}
