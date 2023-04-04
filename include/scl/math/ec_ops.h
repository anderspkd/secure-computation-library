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

#ifndef SCL_MATH_EC_OPS_H
#define SCL_MATH_EC_OPS_H

#include "scl/math/ff.h"
#include "scl/math/number.h"

namespace scl::math {

/**
 * @brief Set a point equal to the point-at-infinity.
 * @param out the point to set to the point-at-infinity.
 */
template <typename C>
void CurveSetPointAtInfinity(typename C::ValueType& out);

/**
 * @brief Check if a point is equal to the point-at-infinity.
 * @param point the point
 * @return true if \p point is equal to the point-at-infinity, otherwise false.
 */
template <typename C>
bool CurveIsPointAtInfinity(const typename C::ValueType& point);

/**
 * @brief Set a point equal to the generator of this curve.
 * @param out the point to set equal to the generator of this curve.
 */
template <typename C>
void CurveSetGenerator(typename C::ValueType& out);

/**
 * @brief Set a point equal to an affine point.
 * @param out the point to set
 * @param x the x coordinate
 * @param y the y coordinate
 */
template <typename C>
void CurveSetAffine(typename C::ValueType& out,
                    const FF<typename C::Field>& x,
                    const FF<typename C::Field>& y);

/**
 * @brief Convert a point to a pair of affine coordinates.
 * @param point the point to convert.
 * @return a set of affine coordinates.
 */
template <typename C>
std::array<FF<typename C::Field>, 2> CurveToAffine(
    const typename C::ValueType& point);

/**
 * @brief Add two elliptic curve points in-place.
 * @param out the first point and output
 * @param in the second point
 */
template <typename C>
void CurveAdd(typename C::ValueType& out, const typename C::ValueType& in);

/**
 * @brief Double an elliptic curve point in-place.
 * @param out the point to double
 */
template <typename C>
void CurveDouble(typename C::ValueType& out);

/**
 * @brief Subtract two elliptic curve points in-place.
 * @param out the first point and output
 * @param in the second point
 */
template <typename C>
void CurveSubtract(typename C::ValueType& out, const typename C::ValueType& in);

/**
 * @brief Negate an elliptic curve point.
 * @param out the point to negate
 */
template <typename C>
void CurveNegate(typename C::ValueType& out);

/**
 * @brief Scalar multiply an elliptic curve point in-place.
 * @param out the point
 * @param scalar the scalar
 */
template <typename C>
void CurveScalarMultiply(typename C::ValueType& out, const Number& scalar);

/**
 * @brief Scalar multiply an elliptic curve point in-place.
 * @param out the point
 * @param scalar the scalar
 */
template <typename C>
void CurveScalarMultiply(typename C::ValueType& out,
                         const FF<typename C::Order>& scalar);

/**
 * @brief Check if two elliptic curve points are equal.
 * @param in1 the first point
 * @param in2 the second point
 */
template <typename C>
bool CurveEqual(const typename C::ValueType& in1,
                const typename C::ValueType& in2);

/**
 * @brief Read an elliptic curve from a byte buffer.
 * @param out where to store the point
 * @param src the buffer
 */
template <typename C>
void CurveFromBytes(typename C::ValueType& out, const unsigned char* src);

/**
 * @brief Write an elliptic curve point to a buffer.
 * @param dest the buffer
 * @param in the elliptic curve point to write
 * @param compress whether to compress the point
 */
template <typename C>
void CurveToBytes(unsigned char* dest,
                  const typename C::ValueType& in,
                  bool compress);

/**
 * @brief Convert an elliptic curve point to a string
 * @param point the point
 * @return an STL string representation of \p in.
 */
template <typename C>
std::string CurveToString(const typename C::ValueType& point);

}  // namespace scl::math

#endif  // SCL_MATH_EC_OPS_H
