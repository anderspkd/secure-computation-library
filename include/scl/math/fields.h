/**
 * @file fields.h
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

#ifndef _SCL_MATH_FIELDS_H
#define _SCL_MATH_FIELDS_H

#include <cstdint>
#include <string>
#include <type_traits>

#include "scl/math/bases.h"

namespace scl {
namespace details {

#define _SCL_STR(x) #x

/**
 * @def DEFINE_FINITE_FIELD(name, bits, internal_type)
 * @brief Helper macro for defining custom Finite Fields.
 *
 * <p>This macro can be used to define a class with all the required methods for
 * working with \ref scl::details::FF.</p>
 *
 * @param[in] finite_field_name the type name of the finite field
 * @param[in] bits the bitsize of the field. Must be unique
 * @param[in] internal_type the type used for internally representing
 elements
 */
#define DEFINE_FINITE_FIELD(finite_field_name, bits, internal_type)          \
  struct finite_field_name {                                                 \
    using ValueType = internal_type;                                         \
    constexpr static const char* kName = _SCL_STR(finite_field_name);        \
    constexpr static const std::size_t kByteSize = sizeof(ValueType);        \
    constexpr static const std::size_t kBitSize = bits;                      \
    static ValueType FromInt(int);                                           \
    static void Add(ValueType&, const ValueType&);                           \
    static void Subtract(ValueType&, const ValueType&);                      \
    static void Multiply(ValueType&, const ValueType&);                      \
    static void Negate(ValueType&);                                          \
    static void Invert(ValueType&);                                          \
    static bool Equal(const ValueType&, const ValueType&);                   \
    static void ToBytes(unsigned char*, const ValueType&);                   \
    static void FromBytes(ValueType&, const unsigned char*);                 \
    static std::string ToString(const ValueType&);                           \
    static void FromString(ValueType&, const std::string&, enum NumberBase); \
  }

#define _SCL_DEFINE_FINITE_FIELD(n, b, t) DEFINE_FINITE_FIELD(n, b, t)

_SCL_DEFINE_FINITE_FIELD(Mersenne61, 61, std::uint64_t);

_SCL_DEFINE_FINITE_FIELD(Mersenne127, 127, __uint128_t);

#define _SCL_LE(a, b) ((a) <= (b))
#define _SCL_GE(a, b) ((a) >= (b))

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
      _SCL_GE(Bits, 1) && _SCL_LE(Bits, 61),
      Mersenne61,

      std::conditional_t<
      _SCL_GE(Bits, 62) && _SCL_LE(Bits, 127),
      Mersenne127,

      void>>;

  // clang-format on
};

#undef _SCL_STR
#undef _SCL_LE
#undef _SCL_GE

}  // namespace details
}  // namespace scl

#endif  // _SCL_MATH_FIELDS_H
