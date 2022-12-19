/**
 * @file str.cc
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

#include "scl/util/str.h"

#include <cstdint>

template <>
std::string scl::details::ToHexString(const __uint128_t& v) {
  std::string str;
  if (v == 0) {
    str = "0";
  } else {
    std::stringstream ss;
    auto top = static_cast<std::uint64_t>(v >> 64);
    auto bot = static_cast<std::uint64_t>(v);
    ss << std::hex;
    if (top > 0) {
      ss << top;
    }
    ss << bot;
    str = ss.str();
  }
  return str;
}
