/**
 * @file field_ops.h
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

#ifndef _SCL_MATH_FIELD_OPS_H
#define _SCL_MATH_FIELD_OPS_H

#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>

#include "scl/math/bases.h"
#include "scl/math/str.h"

namespace scl {
namespace details {

/**
 * @brief Convert an int into a field element.
 * @param out where to store the converted element
 * @param value the integer to convert
 */
template <typename F>
void FieldConvertIn(typename F::ValueType& out, const int value);

/**
 * @brief Add two field elements in-place.
 * @param out the first operand and output
 * @param op the second operand
 */
template <typename F>
void FieldAdd(typename F::ValueType& out, const typename F::ValueType& op);

/**
 * @brief Subtract two field elements in-place.
 * @param out the first operand and output
 * @param op the second operand
 */
template <typename F>
void FieldSubtract(typename F::ValueType& out, const typename F::ValueType& op);

/**
 * @brief Multiply two field elements in-place.
 * @param out the first operand and output
 * @param op the second operand
 */
template <typename F>
void FieldMultiply(typename F::ValueType& out, const typename F::ValueType& op);

/**
 * @brief Negate a field element in-place.
 * @param out the element to negate
 */
template <typename F>
void FieldNegate(typename F::ValueType& out);

/**
 * @brief Invert a field element in-place.
 * @param out the element to invert
 */
template <typename F>
void FieldInvert(typename F::ValueType& out);

/**
 * @brief Check if two field elements are the same.
 * @param in1 the first element
 * @param in2 the second element
 * @return true if \p in1 and \p in2 are the same and false otherwise
 */
template <typename F>
bool FieldEqual(const typename F::ValueType& in1,
                const typename F::ValueType& in2);

/**
 * @brief Convert a field element to bytes.
 *
 * For types that are trivially copyable, this function has a default
 * implementation based on std::memcpy.
 *
 * @param dest the field element to convert
 * @param src where to store the converted element
 */
template <typename F,
          std::enable_if_t<std::is_trivially_copyable_v<typename F::ValueType>,
                           int> = 0>
void FieldToBytes(unsigned char* dest, const typename F::ValueType& src) {
  std::memcpy(dest, &src, sizeof(typename F::ValueType));
}

/**
 * @brief Convert the content of a buffer to a field element.
 * @param dest where to store the converted element
 * @param src the buffer
 */
template <typename F>
void FieldFromBytes(typename F::ValueType& dest, const unsigned char* src);

/**
 * @brief Convert a field element to a string.
 * @param in the field element to convert
 * @return an STL string representation of \p in.
 */
template <typename F>
std::string FieldToString(const typename F::ValueType& in);

/**
 * @brief Read a field element from a string.
 * @param out where to store the resulting element
 * @param src the string
 * @param base the base that the string is in
 */
template <typename F>
void FieldFromString(typename F::ValueType& out, const std::string& src,
                     enum NumberBase base);

}  // namespace details
}  // namespace scl

#endif  // _SCL_MATH_FIELD_OPS_H
