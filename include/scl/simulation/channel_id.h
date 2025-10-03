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

#pragma once

#include <cstddef>
#include <functional>

namespace scl {

/**
 * @brief Identifier used for Channels.
 */
struct ChannelId final {
  /**
   * @brief ID of the local party.
   */
  std::size_t local;

  /**
   * @brief ID of the remote party.
   */
  std::size_t remote;

  /**
   * @brief Allows comparing two ChannelIds.
   */
  auto operator<=>(const ChannelId& other) const = default;

  /**
   * @brief Turn this ChannelId into a ChannelId from \p remote's POV.
   */
  ChannelId flip() const {
    return ChannelId{remote, local};
  }
};

}  // namespace scl

/// @cond

template <>
struct std::hash<scl::ChannelId> {
  std::size_t operator()(const scl::ChannelId& channel_id) const {
    return channel_id.local ^ (channel_id.remote << 32);
  }
};

/// @endcond
