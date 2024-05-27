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

#ifndef SCL_MATH_CURVES_EC_OPS_H
#define SCL_MATH_CURVES_EC_OPS_H

#include <array>
#include <string>

#include "scl/math/ff.h"
#include "scl/math/number.h"

namespace scl::math::ec {

/**
 * @brief Set a point equal to the point-at-infinity.
 * @param out the point to set to the point-at-infinity.
 */
template <typename CURVE>
void setPointAtInfinity(typename CURVE::ValueType& out);

/**
 * @brief Check if a point is equal to the point-at-infinity.
 * @param point the point
 * @return true if \p point is equal to the point-at-infinity, otherwise false.
 */
template <typename CURVE>
bool isPointAtInfinity(const typename CURVE::ValueType& point);

/**
 * @brief Set a point equal to the generator of this curve.
 * @param out the point to set equal to the generator of this curve.
 */
template <typename CURVE>
void setGenerator(typename CURVE::ValueType& out);

/**
 * @brief Set a point equal to an affine point.
 * @param out the point to set
 * @param x the x coordinate
 * @param y the y coordinate
 */
template <typename CURVE>
void setAffine(typename CURVE::ValueType& out,
               const FF<typename CURVE::Field>& x,
               const FF<typename CURVE::Field>& y);

/**
 * @brief Convert a point to a pair of affine coordinates.
 * @param point the point to convert.
 * @return a set of affine coordinates.
 */
template <typename CURVE>
std::array<FF<typename CURVE::Field>, 2> toAffine(
    const typename CURVE::ValueType& point);

/**
 * @brief Add two elliptic curve points in-place.
 * @param out the first point and output
 * @param in the second point
 */
template <typename CURVE>
void add(typename CURVE::ValueType& out, const typename CURVE::ValueType& in);

/**
 * @brief Double an elliptic curve point in-place.
 * @param out the point to double
 */
template <typename CURVE>
void dbl(typename CURVE::ValueType& out);

/**
 * @brief Subtract two elliptic curve points in-place.
 * @param out the first point and output
 * @param in the second point
 */
template <typename CURVE>
void subtract(typename CURVE::ValueType& out,
              const typename CURVE::ValueType& in);

/**
 * @brief Negate an elliptic curve point.
 * @param out the point to negate
 */
template <typename CURVE>
void negate(typename CURVE::ValueType& out);

/**
 * @brief Scalar multiply an elliptic curve point in-place.
 * @param out the point
 * @param scalar the scalar
 */
template <typename CURVE>
void scalarMultiply(typename CURVE::ValueType& out, const Number& scalar);

/**
 * @brief Scalar multiply an elliptic curve point in-place.
 * @param out the point
 * @param scalar the scalar
 */
template <typename CURVE>
void scalarMultiply(typename CURVE::ValueType& out,
                    const FF<typename CURVE::Scalar>& scalar);

/**
 * @brief Check if two elliptic curve points are equal.
 * @param in1 the first point
 * @param in2 the second point
 */
template <typename CURVE>
bool equal(const typename CURVE::ValueType& in1,
           const typename CURVE::ValueType& in2);

/**
 * @brief Read an elliptic curve from a byte buffer.
 * @param out where to store the point
 * @param src the buffer
 */
template <typename CURVE>
void fromBytes(typename CURVE::ValueType& out, const unsigned char* src);

/**
 * @brief Write an elliptic curve point to a buffer.
 * @param dest the buffer
 * @param in the elliptic curve point to write
 * @param compress whether to compress the point
 */
template <typename CURVE>
void toBytes(unsigned char* dest,
             const typename CURVE::ValueType& in,
             bool compress);

/**
 * @brief Convert an elliptic curve point to a string
 * @param point the point
 * @return an STL string representation of \p in.
 */
template <typename CURVE>
std::string toString(const typename CURVE::ValueType& point);

}  // namespace scl::math::ec

#endif  // SCL_MATH_CURVES_EC_OPS_H
