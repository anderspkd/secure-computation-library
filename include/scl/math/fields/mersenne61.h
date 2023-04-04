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

#ifndef SCL_MATH_FIELDS_MERSENNE61_H
#define SCL_MATH_FIELDS_MERSENNE61_H

#include <cstddef>
#include <cstdint>

namespace scl::math {

/**
 * @brief The field \f$\mathbb{F}_p\f$ with \f$p=2^{61}-1\f$.
 */
struct Mersenne61 {
  /**
   * @brief Internal type elements of this field.
   */
  using ValueType = std::uint64_t;

  /**
   * @brief The name of this field.
   */
  constexpr static const char* kName = "Mersenne61";

  /**
   * @brief The size of field elements of this field in bytes.
   */
  constexpr static const std::size_t kByteSize = sizeof(ValueType);

  /**
   * @brief The size of field elements of this field in bits.
   */
  constexpr static const std::size_t kBitSize = 61;
};

}  // namespace scl::math

#endif  // SCL_MATH_FIELDS_MERSENNE61_H
