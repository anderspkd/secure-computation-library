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

#include "scl/hex.h"

#include <cstdint>
#include <iomanip>

template <>
std::string scl::details::toHexString(const __uint128_t& v) {
  auto top = static_cast<std::uint64_t>(v >> 64);
  auto bot = static_cast<std::uint64_t>(v);

  std::stringstream ss;
  ss << std::hex << std::setfill('0');
  if (top) {
    ss << top;
    // have to add appropriate padding to the bottom word here
    ss << std::setw(16);
    ss << bot;
  } else {
    ss << bot;
  }

  auto str = ss.str();
  // make sure the output has an even length
  return str.length() % 2 ? '0' + str : str;
}
