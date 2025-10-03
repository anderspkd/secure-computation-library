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

#ifndef SCL_HEX_H
#define SCL_HEX_H

#include <iomanip>
#include <sstream>
#include <string>

namespace scl::details {

/**
 * @brief Convert value into a string.
 * @ingroup util
 *
 * This function converts a "simple" value into a hex string. It is used to
 * print certain arithmetic types in SCL that are really just wrappers around,
 * say an unsigned int.
 */
template <typename T>
std::string toHexString(const T& v) {
  std::stringstream ss;
  ss << std::hex << v;
  return ss.str();
}

/**
 * @brief Specialization for <code>__uint128_t</code>.
 * @ingroup util
 */
template <>
std::string toHexString(const __uint128_t& v);

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


#define SCL_TO_HEX(v, c)                                                \
  do {                                                                  \
    if ((c) >= '0' && (c) <= '9')                                       \
      (v) += (c) - '0';                                                 \
    else if ((c) >= 'a' && (c) <= 'f')                                  \
      (v) += (c) - 'a' + 10;                                            \
    else if ((c) >= 'A' && (c) <= 'F')                                  \
      (v) += (c) - 'A' + 10;                                            \
    else                                                                \
      throw std::invalid_argument("encountered invalid hex character"); \
  } while (0)

/**
 * @brief Convert a string in hex.
 * @tparam T the output type
 *
 * Normal conventions for hex strings apply, although the <code>0x</code> prefix
 * is not permitted. The input is assumed to encode an integer in big endian.
 */
template <typename T>
T fromHexString(const std::string& s) {
  auto n = s.size();
  if (n % 2) {
    throw std::invalid_argument("odd-length hex string");
  }
  T t = 0;
  for (std::size_t i = 0; i < n; i += 2) {
    char c0 = s[i];
    char c1 = s[i + 1];
    t = t << 4;
    SCL_TO_HEX(t, c0);
    t = t << 4;
    SCL_TO_HEX(t, c1);
  }
  return t;
}

#undef SCL_TO_HEX

}  // namespace scl::details

#endif  // SCL_HEX_H
