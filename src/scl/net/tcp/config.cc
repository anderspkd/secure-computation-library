/* SCL --- Secure Computation Library
 * Copyright (C) 2025 Anders Dalskov
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

#include "scl/net/tcp/config.h"

#include <fstream>
#include <stdexcept>
#include <string>

scl::NetworkConfig scl::NetworkConfig::load(std::size_t my_id,
                                            const std::string& filename) {
  std::ifstream file(filename);

  if (!file.is_open()) {
    throw std::invalid_argument("could not open file");
  }

  std::string line;
  std::vector<ConnectionInfo> info;

  std::size_t i = 0;

  while (std::getline(file, line)) {
    auto split = line.find(',');
    
    if (split == std::string::npos) {
      throw std::invalid_argument("invalid entry in config file");
    }

    auto s = static_cast<std::string::difference_type>(split);
    auto host = std::string(line.begin(), line.begin() + s);
    auto port = std::stoul(std::string(line.begin() + s + 1, line.end()));

    info.emplace_back(ConnectionInfo{i++, host, port});
  }

  if (my_id >= info.size()) {
    throw std::invalid_argument("invalid id");
  }

  return NetworkConfig(my_id, info);
}

scl::NetworkConfig scl::NetworkConfig::localhost(std::size_t my_id,
                                                 std::size_t size,
                                                 std::size_t port_base) {
  if (my_id >= size) {
    throw std::invalid_argument("invalid id");
  }

  std::vector<ConnectionInfo> info;
  for (std::size_t i = 0; i < size; ++i) {
    std::size_t port = port_base + i;
    info.emplace_back(ConnectionInfo{i, "127.0.0.1", port});
  }

  return NetworkConfig(my_id, info);
}

void scl::NetworkConfig::validate() {
  auto n = networkSize();

  if (static_cast<std::size_t>(id()) >= n) {
    throw std::invalid_argument("my ID is invalid in config");
  }

  for (std::size_t i = 0; i < n; ++i) {
    auto pi = m_parties[i];
    if (static_cast<std::size_t>(pi.id) >= n) {
      throw std::invalid_argument("invalid ID in config");
    }
    for (std::size_t j = i + 1; j < n; ++j) {
      auto pj = m_parties[j];
      if (pi.id == pj.id) {
        throw std::invalid_argument("config has duplicate party ids");
      }
    }
  }
}
