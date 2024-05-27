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

#ifndef SCL_SIMULATION_CHANNEL_ID_H
#define SCL_SIMULATION_CHANNEL_ID_H

#include <cstdint>
#include <functional>
#include <ostream>

namespace scl::sim {

/**
 * @brief Channel identifier.
 *
 * <p>During simulations, each pair of parties are connected by two channels
 * <code>{i, j}</code> and <code>{j, i}</code>. The channel <code>{i, j}</code>
 * is used by <code>i</code> when writing to <code>j</code>.
 *
 * <p>ChannelId is <code>==</code> and <code><</code> comparable, and a
 * specialization for std::hash exists. ChannelId can therefore be used as a key
 * in a std::map.
 */
struct ChannelId {
  /**
   * @brief ID of this party.
   */
  std::size_t local;

  /**
   * @brief ID of the remote party.
   */
  std::size_t remote;

  /**
   * @brief Flips the view of the this ID.
   */
  ChannelId flip() const {
    return {remote, local};
  }

  /**
   * @brief operator == for ChannelIds.
   */
  friend bool operator==(const ChannelId& cid0, const ChannelId& cid1) {
    return cid0.local == cid1.local && cid0.remote == cid1.remote;
  }

  /**
   * @brief operator < for ChannelIds.
   */
  friend bool operator<(const ChannelId& cid0, const ChannelId& cid1) {
    return cid0.local < cid1.local ||
           (cid0.local == cid1.local && cid0.remote < cid1.remote);
  }

  /**
   * @brief Print operator for ChannelId.
   */
  friend std::ostream& operator<<(std::ostream& os, const ChannelId& cid) {
    return os << "{local=" << cid.local << ", remote=" << cid.remote << "}";
  }
};

}  // namespace scl::sim

/// @cond

template <>
struct std::hash<scl::sim::ChannelId> {
  std::size_t operator()(const scl::sim::ChannelId& cid) const {
    return cid.local ^ (cid.remote << 32);
  }
};

/// @endcond

#endif  // SCL_SIMULATION_CHANNEL_ID_H
