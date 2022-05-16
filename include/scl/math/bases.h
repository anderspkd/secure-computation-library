/**
 * @file bases.h
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

#ifndef _SCL_MATH_BASES_H
#define _SCL_MATH_BASES_H

namespace scl {

/**
 * @brief Number bases. Used when converting strings to values.
 */
enum class NumberBase {
  BINARY = 2,
  DECIMAL = 10,
  HEX = 16,
  BASE64 = 64,
};

}  // namespace scl

#endif  // _SCL_MATH_BASES_H
