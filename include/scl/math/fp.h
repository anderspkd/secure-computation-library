/**
 * @file fp.h
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

#ifndef SCL_MATH_FP_H
#define SCL_MATH_FP_H

#include <type_traits>

#include "scl/math/ff.h"
#include "scl/math/fields/mersenne127.h"
#include "scl/math/fields/mersenne61.h"

namespace scl {
namespace details {

#define SCL_IN_RANGE(v, l, u) ((l) <= (v) && (v) <= (u))

/**
 * @brief Select a suitable Finite Field based on a provided bitlevel.
 */
template <unsigned Bits>
struct FieldSelector {
  /**
   * @brief The field.
   */
  // clang-format off
  using Field =
      std::conditional_t<
      SCL_IN_RANGE(Bits, 1, 61),
      Mersenne61,

      std::conditional_t<
      SCL_IN_RANGE(Bits, 62, 127),
      Mersenne127,

      void>>;
  // clang-format on
};

#undef SCL_IN_RANGE

}  // namespace details

/**
 * @brief A suitable pre-defined field with space for \p Bits computation.
 *
 * <p>scl::FF picks a suitable pre-defined finite field implementation based on
 * the number of bits of computation the user wants. At the moment, there are
 * two different fields supported: One based on a 61-bit Mersenne prime, and one
 * based on a 127-bit mersenne prime. Depending on \p Bits, the smaller of the
 * fields will be picked and a compile-time error is thrown if \p Bits exceed
 * 127.</p>
 */
template <unsigned Bits>
using Fp = FF<typename details::FieldSelector<Bits>::Field>;

}  // namespace scl

#endif  // SCL_MATH_FP_H
