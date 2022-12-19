/**
 * @file config.cc
 *
 * SCL --- Secure Computation Library
 * Copyright (C) 2022 Anders Dalskov
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

#include "scl/net/config.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

void ValidateIdAndSize(unsigned id, std::size_t n) {
  if (n == 0) {
    throw std::invalid_argument("n cannot be zero");
  }

  if (n <= id) {
    throw std::invalid_argument("invalid id");
  }
}

}  // namespace

scl::NetworkConfig scl::NetworkConfig::Load(int id,
                                            const std::string& filename) {
  std::ifstream file(filename);

  if (!file.is_open()) {
    throw std::invalid_argument("could not open file");
  }

  std::string line;
  std::vector<scl::Party> info;

  while (std::getline(file, line)) {
    auto a_ = line.find(',');
    auto b_ = line.rfind(',');

    if (a_ == std::string::npos || a_ == b_) {
      throw std::invalid_argument("invalid entry in config file");
    }

    auto a = static_cast<std::string::difference_type>(a_);
    auto b = static_cast<std::string::difference_type>(b_);

    auto id = std::stoi(std::string(line.begin(), line.begin() + a));
    auto hostname = std::string(line.begin() + a + 1, line.begin() + b);
    auto port = std::stoi(std::string(line.begin() + b + 1, line.end()));
    info.emplace_back(Party{id, hostname, port});
  }

  ValidateIdAndSize(id, info.size());

  return NetworkConfig(id, info);
}

scl::NetworkConfig scl::NetworkConfig::Localhost(int id,
                                                 int size,
                                                 int port_base) {
  ValidateIdAndSize(id, size);

  std::vector<scl::Party> info;
  for (int i = 0; i < size; ++i) {
    int port = port_base + i;
    info.emplace_back(Party{i, "127.0.0.1", port});
  }

  return NetworkConfig(id, info);
}

std::string scl::NetworkConfig::ToString() const {
  std::stringstream ss;
  ss << "[id=" << mId << ", ";
  std::size_t i = 0;
  for (; i < mParties.size() - 1; i++) {
    const auto party = mParties[i];
    ss << "{" << party.id << ", " << party.hostname << ", " << party.port
       << "}, ";
  }
  const auto last = mParties[i];
  ss << "{" << last.id << ", " << last.hostname << ", " << last.port << "}]";
  return ss.str();
}

void scl::NetworkConfig::Validate() {
  auto n = NetworkSize();

  if (static_cast<std::size_t>(Id()) >= n) {
    throw std::invalid_argument("my ID is invalid in config");
  }

  for (std::size_t i = 0; i < n; ++i) {
    auto pi = mParties[i];
    if (static_cast<std::size_t>(pi.id) >= n) {
      throw std::invalid_argument("invalid ID in config");
    }
    for (std::size_t j = i + 1; j < n; ++j) {
      auto pj = mParties[j];
      if (pi.id == pj.id) {
        throw std::invalid_argument("config has duplicate party ids");
      }
    }
  }
}
