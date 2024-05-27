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

#ifndef SCL_MATH_FIELDS_FF_OPS_H
#define SCL_MATH_FIELDS_FF_OPS_H

#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>

#include "scl/math/number.h"

namespace scl::math::ff {

/**
 * @brief Convert an int into a field element.
 * @param out where to store the converted element
 * @param value the integer to convert
 */
template <typename FIELD>
void convertTo(typename FIELD::ValueType& out, int value);

/**
 * @brief Read a field element from a string.
 * @param out where to store the resulting element
 * @param src the string
 */
template <typename FIELD>
void convertTo(typename FIELD::ValueType& out, const std::string& src);

/**
 * @brief Add two field elements in-place.
 * @param out the first operand and output
 * @param op the second operand
 */
template <typename FIELD>
void add(typename FIELD::ValueType& out, const typename FIELD::ValueType& op);

/**
 * @brief Subtract two field elements in-place.
 * @param out the first operand and output
 * @param op the second operand
 */
template <typename FIELD>
void subtract(typename FIELD::ValueType& out,
              const typename FIELD::ValueType& op);

/**
 * @brief Multiply two field elements in-place.
 * @param out the first operand and output
 * @param op the second operand
 */
template <typename FIELD>
void multiply(typename FIELD::ValueType& out,
              const typename FIELD::ValueType& op);

/**
 * @brief Negate a field element in-place.
 * @param out the element to negate
 */
template <typename FIELD>
void negate(typename FIELD::ValueType& out);

/**
 * @brief Invert a field element in-place.
 * @param out the element to invert
 */
template <typename FIELD>
void invert(typename FIELD::ValueType& out);

/**
 * @brief Check if two field elements are the same.
 * @param in1 the first element
 * @param in2 the second element
 * @return true if \p in1 and \p in2 are the same and false otherwise
 */
template <typename FIELD>
bool equal(const typename FIELD::ValueType& in1,
           const typename FIELD::ValueType& in2);

/**
 * @brief Convert a field element to bytes.
 * @param dest the field element to convert
 * @param src where to store the converted element
 */
template <typename FIELD>
void toBytes(unsigned char* dest, const typename FIELD::ValueType& src);

/**
 * @brief Convert the content of a buffer to a field element.
 * @param dest where to store the converted element
 * @param src the buffer
 */
template <typename FIELD>
void fromBytes(typename FIELD::ValueType& dest, const unsigned char* src);

/**
 * @brief Convert a field element to a string.
 * @param in the field element to convert
 * @return an STL string representation of \p in.
 */
template <typename FIELD>
std::string toString(const typename FIELD::ValueType& in);

}  // namespace scl::math::ff

#endif  // SCL_MATH_FIELDS_FF_OPS_H
