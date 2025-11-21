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

#include "scl/simulation/params.h"

#include <cmath>
#include <cstddef>
#include <limits>
#include <unordered_map>

#include "scl/simulation/channel_id.h"

using namespace scl;

std::size_t ChannelParams::bandwidth() {
  if (deterministicChannel()) {
    auto& p = std::get<DetParams>(m_params);
    return p.bandwidth;
  } else {
    auto& p = std::get<PropParams>(m_params);
    return std::lround(p.bandwidth(p.rg));
  }
}

std::size_t ChannelParams::latency() {
  if (deterministicChannel()) {
    auto& p = std::get<DetParams>(m_params);
    return p.latency;
  } else {
    auto& p = std::get<PropParams>(m_params);
    return std::lround(p.latency(p.rg));
  }
}

NetworkParams NetworkParams::create(std::size_t number_of_parties,
                                    std::size_t bandwidth,
                                    std::size_t latency,
                                    float packet_loss) {
  std::unordered_map<ChannelId, ChannelParams> params;

  for (std::size_t i = 0; i < number_of_parties; i++) {
    params[ChannelId(i, i)] =
        ChannelParams::createDet(std::numeric_limits<std::size_t>::max(), 0, 0);
    for (std::size_t j = i + 1; j < number_of_parties; j++) {
      params[ChannelId(i, j)] =
          ChannelParams::createDet(bandwidth, latency, packet_loss);
      params[ChannelId(j, i)] =
          ChannelParams::createDet(bandwidth, latency, packet_loss);
    }
  }

  return NetworkParams{params};
}
