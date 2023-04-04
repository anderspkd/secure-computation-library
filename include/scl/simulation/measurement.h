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
 *
 * <p>A measurement holds the raw samples that are extracted from a protocol,
 * but provides several functions that derive useful statistics about these
 * samples. Provided statistics are:
 *
 * <ul>
 * <li>Mean() and Median()</li>
 * <li>Min() and Max()</li>
 * <li>StdDev() i.e., standard deviation</li>
 * </ul>
 */
template <typename T>
class Measurement {
 public:
  /**
   * @brief Add a sample to this measurement.
   * @param sample the sample.
   */
  void AddSample(const T& sample) {
    mSamples.emplace_back(sample);
    // Maybe it makes more sense to use a datastructure here where insertion
    // anywhere is constant time?
    std::sort(mSamples.begin(), mSamples.end());
  }

  /**
   * @brief Read-only access to the samples in this measurement.
   * @return the samples.
   */
  const std::vector<T>& Samples() const {
    return mSamples;
  }

  /**
   * @brief The size of this measurement, defined as the number of samples.
   * @return number of samples.
   */
  std::size_t Size() const {
    return mSamples.size();
  }

  /**
   * @brief Check whether this measurement is empty.
   * @return true if this measurement has zero samples, and false otherwise.
   */
  bool Empty() const {
    return mSamples.empty();
  }

  /**
   * @brief Mean.
   * @return mean of the samples in this measurement.
   */
  T Mean() const;

  /**
   * @brief Median.
   * @return median of the samples in this measurement.
   */
  T Median() const;

  /**
   * @brief Mininum.
   * @return smallest observed sample.
   */
  T Min() const {
    return Empty() ? Zero() : mSamples[0];
  }

  /**
   * @brief Maximum.
   * @return largest observed sample.
   */
  T Max() const {
    return Empty() ? Zero() : mSamples[Size() - 1];
  }

  /**
   * @brief Standard deviation.
   * @return standard deviation of the samples in this measurement.
   */
  T StdDev() const;

 private:
  std::vector<T> mSamples;

  static T Zero() {
    return 0;
  }

  static T Sqrt(const T& v) {
    return std::sqrt(v);
  }

  static T Sqr(const T& v) {
    return v * v;
  }
};

template <typename T>
T Measurement<T>::Mean() const {
  T sum = Zero();
  for (const auto& v : mSamples) {
    sum += v;
  }
  return sum / Size();
}

template <typename T>
T Measurement<T>::Median() const {
  if (Empty()) {
    return Zero();
  }

  if (Size() == 1) {
    return mSamples[0];
  }

  const auto i = Size() / 2;
  if (Size() % 2 == 0) {
    return mSamples[i];
  }

  return mSamples[i] + mSamples[i + 1];
}

template <typename T>
T Measurement<T>::StdDev() const {
  const auto mu = Mean();
  auto sum = Zero();
  for (const auto& v : mSamples) {
    sum += Sqr(v - mu);
  }
  return Sqrt(sum / Size());
}

template <>
inline util::Time::Duration Measurement<util::Time::Duration>::Zero() {
  return util::Time::Duration::zero();
}

template <>
inline util::Time::Duration Measurement<util::Time::Duration>::Sqrt(
    const util::Time::Duration& v) {
  long double u = std::sqrt(v.count());
  std::chrono::duration<long double, util::Time::Duration::period> w(u);
  return std::chrono::duration_cast<util::Time::Duration>(w);
}

template <>
inline util::Time::Duration Measurement<util::Time::Duration>::Sqr(
    const util::Time::Duration& v) {
  long double u = v.count();
  std::chrono::duration<long double, util::Time::Duration::period> w(u * u);
  return std::chrono::duration_cast<util::Time::Duration>(w);
}

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
