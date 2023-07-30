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

#ifndef SCL_SIMULATION_MEASUREMENT_H
#define SCL_SIMULATION_MEASUREMENT_H

#include <algorithm>
#include <cmath>
#include <ostream>
#include <vector>

#include "scl/util/time.h"

namespace scl::sim {

/**
 * @brief Measurement from a simulation.
 */
template <typename T>
class Measurement {
 public:
  /**
   * @brief Add a sample to this measurement.
   * @param sample the sample.
   */
  void AddSample(const T& sample) {
    m_samples.emplace_back(sample);
  }

  /**
   * @brief Read-only access to the samples in this measurement.
   * @return the samples.
   */
  std::vector<T> Samples() const {
    return m_samples;
  }

  /**
   * @brief The size of this measurement, defined as the number of samples.
   * @return number of samples.
   */
  std::size_t Size() const {
    return m_samples.size();
  }

  /**
   * @brief Check whether this measurement is empty.
   * @return true if this measurement has zero samples, and false otherwise.
   */
  bool Empty() const {
    return m_samples.empty();
  }

 private:
  std::vector<T> m_samples;
};

/**
 * @brief A measurement for time related observations.
 *
 * This type holds measurements related to time. In particular, measurements
 * concerning the execution time of protocols and protocol segments. The data
 * type is util::Time::Duration, which is essentially
 * <code>std::chrono::duration</code>.
 */
using TimeMeasurement = Measurement<util::Time::Duration>;

/**
 * @brief Write a TimeMeasurement to an std::ostream.
 */
std::ostream& operator<<(std::ostream& os, const TimeMeasurement& m);

/**
 * @brief A measurement for data related observations.
 *
 * This type holds measurements related to data transfer amounts. That is, the
 * amount of data that is being sent and received in some context.
 */
using DataMeasurement = Measurement<long double>;

/**
 * @brief Write a DataMeasurement to an std::ostream.
 */
std::ostream& operator<<(std::ostream& os, const DataMeasurement& m);

/**
 * @brief A measurement for data sent and received.
 *
 * This wraps two DataMeasurements: One for the data being sent, and one for
 * data being received. This struct thus models e.g., the data that a particular
 * party sends in a segment, or the data being sent on a channel.
 */
struct SendRecvMeasurement {
  /**
   * @brief A measurement for data sent.
   */
  DataMeasurement sent;

  /**
   * @brief A measurement for data received.
   */
  DataMeasurement recv;
};

}  // namespace scl::sim

#endif  // SCL_SIMULATION_MEASUREMENT_H
