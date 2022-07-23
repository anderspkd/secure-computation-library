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

static inline void ValidateIdAndSize(unsigned id, std::size_t n) {
  if (!n) {
    throw std::invalid_argument("n cannot be zero");
  } else if (n <= id) {
    throw std::invalid_argument("invalid id");
  }
}

scl::NetworkConfig scl::NetworkConfig::Load(unsigned id, std::string filename) {
  std::ifstream file(filename);

  if (!file.is_open()) {
    throw std::invalid_argument("could not open file");
  }

  std::string line;
  std::vector<scl::Party> info;

  while (std::getline(file, line)) {
    auto a = line.find(',');
    auto b = line.rfind(',');

    if (a == std::string::npos || a == b) {
      throw std::invalid_argument("invalid entry in config file");
    }

    auto id = (unsigned)std::stoul(std::string(line.begin(), line.begin() + a));
    auto hostname = std::string(line.begin() + a + 1, line.begin() + b);
    auto port = std::stoi(std::string(line.begin() + b + 1, line.end()));
    info.emplace_back(Party{id, hostname, port});
  }

  ValidateIdAndSize(id, info.size());

  return NetworkConfig(id, info);
}

scl::NetworkConfig scl::NetworkConfig::Localhost(unsigned id, std::size_t n,
                                                 int port_base) {
  ValidateIdAndSize(id, n);

  std::vector<scl::Party> info;
  for (std::size_t i = 0; i < n; ++i) {
    int port = port_base + i;
    info.emplace_back(Party{(unsigned)i, "127.0.0.1", port});
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

  if (Id() >= n) {
    throw std::invalid_argument("my ID is invalid in config");
  }

  for (std::size_t i = 0; i < n; ++i) {
    auto pi = mParties[i];
    if (pi.id >= n) {
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
