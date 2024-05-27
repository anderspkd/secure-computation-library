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

#ifndef SCL_MATH_FP_H
#define SCL_MATH_FP_H

#include <type_traits>

#include "scl/math/ff.h"
#include "scl/math/fields/mersenne127.h"
#include "scl/math/fields/mersenne61.h"

namespace scl::math {

#define SCL_IN_RANGE(v, l, u) ((l) <= (v) && (v) <= (u))

/**
 * @brief Select a suitable Finite Field based on a provided bitlevel.
 */
template <std::size_t BITS>
struct FieldSelector {
  static_assert(BITS > 0 && BITS < 128, "Bits not in range [1, 127]");

  /**
   * @brief The field.
   */
  using Field = std::
      conditional_t<SCL_IN_RANGE(BITS, 1, 61), ff::Mersenne61, ff::Mersenne127>;
};

#undef SCL_IN_RANGE

/**
 * @brief A suitable pre-defined field with space for \p Bits computation.
 * @tparam Bits the size in bits of the finite field
 *
 * Fp is an alias for FF instantiated with a prime order Finite Field of an
 * appropriate size indicated by the \p Bits parameter. Currently, only \p Bits
 * have to be in the range <code>[1, 127]</code> as SCL only has support for two
 * fields:
 * <ul>
 * <li>A 61-bit Finite Field defined over a \f$p=2^{61}-1\f$</li>
 * <li>A 127-bit Finite Field defined over a \f$p=2^{127}-1\f$</li>
 * </ul>
 *
 * @see Mersenne61
 * @see Mersenne127
 */
template <std::size_t BITS>
using Fp = FF<typename FieldSelector<BITS>::Field>;

}  // namespace scl::math

#endif  // SCL_MATH_FP_H
