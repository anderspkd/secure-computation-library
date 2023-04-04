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

#include "scl/simulation/config.h"

#include <ostream>
#include <stdexcept>

void scl::sim::SimulatedNetworkConfig::Builder::Validate() const {
  if (mBandwidth.has_value()) {
    if (mBandwidth.value() == 0) {
      throw std::invalid_argument("bandwidth cannot be 0");
    }
  }

  if (mMSS.has_value()) {
    if (mMSS.value() == 0) {
      throw std::invalid_argument("MSS cannot be 0");
    }
  }

  if (mPackageLoss.has_value()) {
    if (mPackageLoss.value() < 0) {
      throw std::invalid_argument("package loss percentage cannot be negative");
    }
    if (mPackageLoss.value() >= 1) {
      throw std::invalid_argument("package loss percentage cannot exceed 100%");
    }
  }

  if (mWindowSize.has_value()) {
    if (mWindowSize.value() == 0) {
      throw std::invalid_argument("TCP window size cannot be 0");
    }
  }
}

std::ostream& scl::sim::operator<<(std::ostream& os,
                                   const SimulatedNetworkConfig& config) {
  os << "SimulationConfig{";
  os << "Bandwidth: " << config.Bandwidth() << " bits/s, ";
  os << "RTT: " << config.RTT() << " ms, ";
  os << "MSS: " << config.MSS() << " bytes, ";
  os << "PackageLoss: " << 100 * config.PackageLoss() << "%, ";
  os << "WindowSize: " << config.WindowSize() << " bytes}";

  return os;
}

scl::sim::SimulatedNetworkConfig scl::sim::SimulatedNetworkConfig::Default() {
  return SimulatedNetworkConfig::Builder{}.Build();
}

scl::sim::SimulatedNetworkConfig scl::sim::SimulatedNetworkConfig::Loopback() {
  return SimulatedNetworkConfig::Builder{}
      .Bandwidth(-1)
      .MSS(-1)
      .PackageLoss(0)
      .RTT(0)
      .WindowSize(-1)
      .Build();
}
