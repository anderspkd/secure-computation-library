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

#ifndef SCL_MATH_EC_H
#define SCL_MATH_EC_H

#include <ostream>

#include "scl/math/ec_ops.h"
#include "scl/math/ff.h"
#include "scl/math/number.h"
#include "scl/math/ops.h"

namespace scl::math {

/**
 * @brief Elliptic Curve interface.
 * @tparam Curve elliptic curve definition
 *
 * TODO.
 *
 * @see FF
 * @see Secp256k1
 */
template <typename Curve>
class EC final : Add<EC<Curve>>, Eq<EC<Curve>>, Print<EC<Curve>> {
 public:
  /**
   * @brief The field that this curve is defined over.
   */
  using Field = FF<typename Curve::Field>;

  /**
   * @brief A large sub-group of this curve.
   */
  using ScalarField = FF<typename Curve::Scalar>;

  /**
   * @brief The size of a curve point in bytes.
   * @param compressed
   */
  constexpr static std::size_t ByteSize(bool compressed = true) {
    return 1 + (compressed ? 0 : Field::ByteSize()) + Field::ByteSize();
  }

  /**
   * @brief The size of a curve point in bits.
   */
  constexpr static std::size_t BitSize(bool compressed = true) {
    return ByteSize(compressed) * 8;
  }

  /**
   * @brief A string indicating which curve this is.
   */
  constexpr static const char* Name() {
    return Curve::NAME;
  }

  /**
   * @brief Get the generator of this curve.
   */
  constexpr static EC Generator() {
    EC g;
    CurveSetGenerator<Curve>(g.m_value);
    return g;
  }  // LCOV_EXCL_LINE

  /**
   * @brief Read an elliptic curve point from bytes.
   * @param src the bytes
   * @return an elliptic curve point.
   */
  static EC Read(const unsigned char* src) {
    EC e;
    CurveFromBytes<Curve>(e.m_value, src);
    return e;
  }  // LCOV_EXCL_LINE

  /**
   * @brief Create a point from an pair of affine coordinates.
   */
  static EC FromAffine(const Field& x, const Field& y) {
    EC e;
    CurveSetAffine<Curve>(e.m_value, x, y);
    return e;
  }

  /**
   * @brief Create a new point equal to the point at infinity.
   */
  explicit constexpr EC() {
    CurveSetPointAtInfinity<Curve>(m_value);
  }

  /**
   * @brief Destructor. Does nothing.
   */
  ~EC() {}

  /**
   * @brief Add another EC point to this.
   * @param other the other point
   * @return this
   */
  EC& operator+=(const EC& other) {
    CurveAdd<Curve>(m_value, other.m_value);
    return *this;
  }

  /**
   * @brief Double this point.
   * @return this after doubling.
   */
  EC& DoubleInPlace() {
    CurveDouble<Curve>(m_value);
    return *this;
  }

  /**
   * @brief Double this point.
   * @return \p this + \p this.
   */
  EC Double() const {
    EC copy(*this);
    return copy.DoubleInPlace();
  }

  /**
   * @brief Subtract another point from this.
   * @param other the other point
   * @return this.
   */
  EC& operator-=(const EC& other) {
    CurveSubtract<Curve>(m_value, other.m_value);
    return *this;
  }

  /**
   * @brief Perform a scalar multiplication.
   * @param scalar the scalar
   * @return this.
   */
  EC& operator*=(const Number& scalar) {
    CurveScalarMultiply<Curve>(m_value, scalar);
    return *this;
  }

  /**
   * @brief Perform a scalar multiplication.
   * @param scalar the scalar
   * @return this.
   */
  EC& operator*=(const ScalarField& scalar) {
    CurveScalarMultiply<Curve>(m_value, scalar);
    return *this;
  }

  /**
   * @brief Multiply a point with a scalar from the right.
   * @param point the point
   * @param scalar the scalar
   * @return the point multiplied with the scalar.
   */
  friend EC operator*(const EC& point, const Number& scalar) {
    EC copy(point);
    return copy *= scalar;
  }

  /**
   * @brief Multiply a point with a scalar from the right.
   * @param point the point
   * @param scalar the scalar
   * @return the point multiplied with the scalar.
   */
  friend EC operator*(const EC& point, const ScalarField& scalar) {
    EC copy(point);
    return copy *= scalar;
  }

  /**
   * @brief Multiply a point with a scalar from the left.
   * @param point the point
   * @param scalar the scalar
   * @return the point multiplied with the scalar.
   */
  friend EC operator*(const Number& scalar, const EC& point) {
    return point * scalar;
  }

  /**
   * @brief Multiply a point with a scalar from the left.
   * @param point the point
   * @param scalar the scalar
   * @return the point multiplied with the scalar.
   */
  friend EC operator*(const ScalarField& scalar, const EC& point) {
    return point * scalar;
  }  // LCOV_EXCL_LINE

  /**
   * @brief Negate this point.
   * @return this.
   */
  EC& Negate() {
    CurveNegate<Curve>(m_value);
    return *this;
  }

  /**
   * @brief Check if this EC point is equal to another EC point.
   * @param other the other EC point
   * @return true if the two points are equal and false otherwise.
   */
  bool Equal(const EC& other) const {
    return CurveEqual<Curve>(m_value, other.m_value);
  }  // LCOV_EXCL_LINE

  /**
   * @brief Check if this point is equal to the point at inifity.
   * @return true if this point is equal to the point at inifity.
   */
  bool PointAtInfinity() const {
    return CurveIsPointAtInfinity<Curve>(m_value);
  }  // LCOV_EXCL_LINE

  /**
   * @brief Return this point as a pair of affine coordinates.
   * @return this point as a pair of affine coordinates.
   */
  std::array<Field, 2> ToAffine() const {
    return CurveToAffine<Curve>(m_value);
  }  // LCOV_EXCL_LINE

  /**
   * @brief Output this point as a string.
   */
  std::string ToString() const {
    return CurveToString<Curve>(m_value);
  }  // LCOV_EXCL_LINE

  /**
   * @brief Write this point to a buffer.
   * @param dest the destination
   * @param compress whether to compress the point
   */
  void Write(unsigned char* dest, bool compress = true) const {
    CurveToBytes<Curve>(dest, m_value, compress);
  }  // LCOV_EXCL_LINE

 private:
  typename Curve::ValueType m_value;
};

/**
 * @brief Helper class for working with finite field elements.
 *
 * This class is a private friend of FF. Specialization of this class therefore
 * allows direct access to the internal representation of a finite field
 * element.
 */
template <typename T>
class FFAccess {};

}  // namespace scl::math

#endif  // SCL_MATH_EC_H
