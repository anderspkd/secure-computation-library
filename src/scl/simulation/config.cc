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

using namespace scl;

void sim::ChannelConfig::Builder::Validate() const {
  if (m_bandwidth.has_value()) {
    if (m_bandwidth.value() == 0) {
      throw std::invalid_argument("bandwidth cannot be 0");
    }
  }

  if (m_MSS.has_value()) {
    if (m_MSS.value() == 0) {
      throw std::invalid_argument("MSS cannot be 0");
    }
  }

  if (m_package_loss.has_value()) {
    if (m_package_loss.value() < 0) {
      throw std::invalid_argument("package loss percentage cannot be negative");
    }
    if (m_package_loss.value() >= 1) {
      throw std::invalid_argument("package loss percentage cannot exceed 100%");
    }
  }

  if (m_window_size.has_value()) {
    if (m_window_size.value() == 0) {
      throw std::invalid_argument("TCP window size cannot be 0");
    }
  }
}

std::ostream& sim::operator<<(std::ostream& os, const ChannelConfig& config) {
  if (config.Type() == sim::ChannelConfig::NetworkType::TCP) {
    os << "SimulationConfig{";
    os << "Type: TCP, ";
    os << "Bandwidth: " << config.Bandwidth() << " bits/s, ";
    os << "RTT: " << config.RTT() << " ms, ";
    os << "MSS: " << config.MSS() << " bytes, ";
    os << "PackageLoss: " << 100 * config.PackageLoss() << "%, ";
    os << "WindowSize: " << config.WindowSize() << " bytes}";
  } else {
    os << "SimulationConfig{INSTANT}";
  }

  return os;
}

sim::ChannelConfig sim::ChannelConfig::Default() {
  return ChannelConfig::Builder{}.Build();
}

sim::ChannelConfig sim::ChannelConfig::Loopback() {
  return ChannelConfig::Builder{}.Type(NetworkType::INSTANT).Build();
}
