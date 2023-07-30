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

#include "scl/simulation/measurement.h"

#include <cmath>

using namespace scl;

namespace {

template <typename T>
T Zero() {
  return 0;
}

template <>
util::Time::Duration Zero() {
  return util::Time::Duration::zero();
}

template <typename T>
T Mean(const sim::Measurement<T>& m) {
  T sum = Zero<T>();
  for (const auto& v : m.Samples()) {
    sum += v;
  }
  return sum / m.Size();
}

long double Sqrt(long double v) {
  return std::sqrt(v);
}

long double Sqr(long double v) {
  return v * v;
}

util::Time::Duration Sqrt(const util::Time::Duration& v) {
  long double u = std::sqrt(v.count());
  std::chrono::duration<long double, util::Time::Duration::period> w(u);
  return std::chrono::duration_cast<util::Time::Duration>(w);
}

util::Time::Duration Sqr(const util::Time::Duration& v) {
  long double u = v.count();
  std::chrono::duration<long double, util::Time::Duration::period> w(u * u);
  return std::chrono::duration_cast<util::Time::Duration>(w);
}

template <typename T>
T StdDev(const sim::Measurement<T>& m) {
  const auto mu = Mean(m);
  auto sum = Zero<T>();
  for (const auto& v : m.Samples()) {
    sum += Sqr(v - mu);
  }
  return Sqrt(sum / m.Size());
}

}  // namespace

std::ostream& sim::operator<<(std::ostream& os, const sim::TimeMeasurement& m) {
  const auto mean = std::chrono::duration<double, std::milli>(Mean(m)).count();
  const auto std_dev =
      std::chrono::duration<double, std::milli>(StdDev(m)).count();

  os << "{"
     << "\"mean\": " << mean << ", "
     << "\"unit\": \"ms\", "
     << "\"std_dev\": " << std_dev << "}";

  return os;
}

std::ostream& sim::operator<<(std::ostream& os, const sim::DataMeasurement& m) {
  const auto mean = Mean(m);
  const auto std_dev = StdDev(m);

  os << "{"
     << "\"mean\": " << mean << ", "
     << "\"unit\": \"B\", "
     << "\"std_dev\": " << std_dev << "}";

  return os;
}
