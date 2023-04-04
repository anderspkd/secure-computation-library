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

#ifndef SCL_SIMULATION_CHANNEL_ID_H
#define SCL_SIMULATION_CHANNEL_ID_H

#include <cstdint>
#include <functional>

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
   * @brief Construct a new channel ID.
   * @param local ID of the this (i.e., the local) party
   * @param remote ID of the remote party
   */
  ChannelId(std::size_t local, std::size_t remote)
      : local(local), remote(remote){};

  /**
   * @brief ID of this party.
   */
  std::size_t local;

  /**
   * @brief ID of the remote party.
   */
  std::size_t remote;

  /**
   * @brief Flip the direction of the channel.
   *
   * Flip turns a channel ID <code>{i, j}</code> into a channel ID <code>{j,
   * i}</code>. This is used when a party <code>i</code> needs the ID of the
   * channel it should read from when receving data from <code>j</code>.
   */
  ChannelId Flip() const {
    return ChannelId{remote, local};
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
  };
};

}  // namespace scl::sim

/// @cond

template <>
struct std::hash<scl::sim::ChannelId> {
  std::size_t operator()(const scl::sim::ChannelId& cid) const {
    return hash<unsigned>{}(cid.local) ^ (hash<unsigned>{}(cid.remote) << 3);
  }
};

/// @endcond

#endif  // SCL_SIMULATION_CHANNEL_ID_H
