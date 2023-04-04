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

#ifndef TEST_SCL_GF7_H
#define TEST_SCL_GF7_H

#include <iostream>
#include <sstream>

namespace scl::test {

struct GaloisField7 {
  using ValueType = unsigned char;
  constexpr static const char* kName = "GF(7)";
  constexpr static const std::size_t kByteSize = 1;
  constexpr static const std::size_t kBitSize = 8;
};

}  // namespace scl::test

#endif /* TEST_SCL_GF7_H */
