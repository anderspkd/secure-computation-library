/**
 * @file util.h
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

#ifndef _TEST_SCL_NET_UTIL_H
#define _TEST_SCL_NET_UTIL_H

#include <cstddef>
namespace scl_tests {

/**
 * @brief The default starting point for ports.
 */
#ifndef _SCL_DEFAULT_TEST_PORT
#define _SCL_DEFAULT_TEST_PORT 14421
#endif

/**
 * @brief Get a fresh port for use in tests that require ports.
 * @note Not thread safe.
 */
int GetPort();

/**
 * @brief Test if two buffers are equal.
 * @param a the first buffer
 * @param b the second buffer
 * @param n the number of bytes to check
 * @param true if \p a and \p b coincide on the first \p n bytes.
 */
bool BufferEquals(const unsigned char* a, const unsigned char* b, int n);

}  // namespace scl_tests

#endif /* _TEST_SCL_NET_UTIL_H */
