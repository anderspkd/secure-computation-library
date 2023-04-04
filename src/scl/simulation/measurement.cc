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

std::ostream& scl::sim::operator<<(std::ostream& os,
                                   const scl::sim::TimeMeasurement& m) {
  auto mean = std::chrono::duration<double, std::milli>(m.Mean()).count();
  auto std_dev = std::chrono::duration<double, std::milli>(m.StdDev()).count();

  os << "{"
     << "\"mean\": " << mean << ", "
     << "\"unit\": \"ms\", "
     << "\"std_dev\": " << std_dev << "}";

  return os;
}

std::ostream& scl::sim::operator<<(std::ostream& os,
                                   const scl::sim::DataMeasurement& m) {
  os << "{"
     << "\"mean\": " << m.Mean() << ", "
     << "\"unit\": \"B\", "
     << "\"std_dev\": " << m.StdDev() << "}";

  return os;
}
