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

#ifndef SCL_HEX_H
#define SCL_HEX_H

#include <iomanip>
#include <sstream>
#include <string>

namespace scl {

/**
 * @brief Convert value into a string.
 */
template <typename T>
std::string toHexString(const T& v) {
  std::stringstream ss;
  ss << std::hex << v;
  return ss.str();
}

/**
 * @brief Convert a list of bytes to a string.
 * @param begin the start of an iterator
 * @param end the end of an iterator
 * @return a hex representation of the digest.
 */
template <typename It>
std::string toHexString(It begin, It end) {
  std::stringstream ss;
  ss << std::setfill('0') << std::hex;
  while (begin != end) {
    ss << std::setw(2) << static_cast<int>(*begin++);
  }
  return ss.str();
}

/**
 * @brief ToHexString specialization for <code>__uint128_t</code>.
 */
template <>
std::string toHexString(const __uint128_t& v);

}  // namespace scl

#endif  // SCL_HEX_H
