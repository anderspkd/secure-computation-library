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

#ifndef SCL_UTIL_MEASUREMENT_H
#define SCL_UTIL_MEASUREMENT_H

#include <algorithm>
#include <cmath>
#include <ostream>
#include <vector>

#include "scl/util/time.h"

namespace scl::util {

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
  void addSample(const T& sample) {
    m_samples.emplace_back(sample);
  }

  /**
   * @brief Read-only access to the samples in this measurement.
   * @return the samples.
   */
  std::vector<T> samples() const {
    return m_samples;
  }

  /**
   * @brief Begin iterator to the samples.
   */
  auto begin() const {
    return m_samples.begin();
  }

  /**
   * @brief End iterator to the samples.
   */
  auto end() const {
    return m_samples.end();
  }

  /**
   * @brief Get the mean of the measurements.
   */
  T mean() const {
    T sum = zero();
    for (const auto& v : m_samples) {
      sum += v;
    }
    return sum / size();
  }

  /**
   * @brief Get the variance of the measurement.
   */
  T var() const {
    // exit early to avoid a division by 0 later.
    if (size() <= 1) {
      return zero();
    }

    const T mu = mean();
    T sum = zero();
    for (const auto& v : m_samples) {
      sum += square(v - mu);
    }
    return sum / (size() - 1);
  }

  /**
   * @brief Get the median of the measurement.
   */
  T median() const {
    if (empty()) {
      return zero();
    }

    const std::size_t half = size() / 2;

    if (size() % 2 == 1) {
      return m_samples[half];
    }

    return (m_samples[half] + m_samples[half - 1]) / 2;
  }

  /**
   * @brief Get the sample standard deviation of the measurements.
   */
  T stddev() const {
    return sqrt(var());
  }

  /**
   * @brief The size of this measurement, defined as the number of samples.
   * @return number of samples.
   */
  std::size_t size() const {
    return m_samples.size();
  }

  /**
   * @brief Check whether this measurement is empty.
   * @return true if this measurement has zero samples, and false otherwise.
   */
  bool empty() const {
    return m_samples.empty();
  }

 private:
  std::vector<T> m_samples;

  // helpers for the stddev() function.
  T square(T val) const;
  T sqrt(T val) const;
  T zero() const;
};

/**
 * @brief A measurement for time related observations.
 */
using TimeMeasurement = Measurement<util::Time::Duration>;

/**
 * @brief Write a TimeMeasurement to an std::ostream.
 */
std::ostream& operator<<(std::ostream& os, const TimeMeasurement& m);

/**
 * @brief A measurement for data related observations.
 */
using DataMeasurement = Measurement<long double>;

/**
 * @brief Write a DataMeasurement to an std::ostream.
 */
std::ostream& operator<<(std::ostream& os, const DataMeasurement& m);

/**
 * @brief A measurement for data sent and received.
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

}  // namespace scl::util

#endif  // SCL_UTIL_MEASUREMENT_H
