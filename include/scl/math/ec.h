/* SCL --- Secure Computation Library
 * Copyright (C) 2025 Anders Dalskov
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

#include <array>
#include <ostream>
#include <string>

#include "scl/math/ec_ops.h"
#include "scl/math/ff.h"
#include "scl/math/number.h"

namespace scl {

/**
 * @brief Elliptic Curve interface.
 * @tparam CURVE elliptic curve definition
 *
 * EC defines a point \f$P\f$ on some Elliptic Curve \f$E(K)\f$. The
 * curve parameters is defined through the \p CURVE template parameter and
 * appropriate overloads of the functions in the \ref ec namespace.
 */
template <typename CURVE>
class EC final {
 public:
  /**
   * @brief Field that this curve is defined over.
   */
  using Field = FF<typename CURVE::Field>;

  /**
   * @brief Large subgroup of this curve.
   */
  using ScalarField = FF<typename CURVE::Scalar>;

  /**
   * @brief Size of a curve point in bytes.
   */
  constexpr static std::size_t byteSize(bool compressed) {
    return 1 + (compressed ? 0 : Field::byteSize()) + Field::byteSize();
  }

  /**
   * @brief Size of a curve point in bits.
   */
  constexpr static std::size_t bitSize(bool compressed) {
    return byteSize(compressed) * 8;
  }

  /**
   * @brief String indicating which curve this is.
   */
  constexpr static const char* name() {
    return CURVE::NAME;
  }

  /**
   * @brief A generator of this curve.
   */
  constexpr static EC generator() {
    EC g;
    details::setGenerator<CURVE>(g.m_value);
    return g;
  }

  /**
   * @brief Reads an elliptic curve point from bytes.
   */
  static EC read(const unsigned char* src) {
    EC e;
    details::fromBytes<CURVE>(e.m_value, src);
    return e;
  }

  /**
   * @brief Creates a point from a pair of affine coordinates.
   */
  static EC fromAffine(const Field& x, const Field& y) {
    EC e;
    details::setAffine<CURVE>(e.m_value, x, y);
    return e;
  }

  /**
   * @brief Get the additive identity of this curve.
   */
  static EC zero() {
    return EC{};
  }

  /**
   * @brief Create a new point equal to the point at infinity.
   */
  EC() {
    details::setPointAtInfinity<CURVE>(m_value);
  }

  /**
   * @brief Destructor. Does nothing.
   */
  ~EC() {}

  /**
   * @brief Add another EC point to this.
   */
  EC& operator+=(const EC& other) {
    details::add<CURVE>(m_value, other.m_value);
    return *this;
  }

  /**
   * @brief Add two curve points.
   */
  friend EC operator+(const EC& lhs, const EC& rhs) {
    EC tmp(lhs);
    return tmp += rhs;
  }

  /**
   * @brief Double this point.
   */
  EC& doublePointInPlace() {
    details::dbl<CURVE>(m_value);
    return *this;
  }

  /**
   * @brief Double this point.
   */
  EC doublePoint() const {
    EC copy(*this);
    return copy.doublePointInPlace();
  }

  /**
   * @brief Subtract another point from this.
   */
  EC& operator-=(const EC& other) {
    details::subtract<CURVE>(m_value, other.m_value);
    return *this;
  }

  /**
   * @brief Subtract two curve points.
   */
  friend EC operator-(const EC& lhs, const EC& rhs) {
    EC tmp(lhs);
    return tmp -= rhs;
  }

  /**
   * @brief Perform a scalar multiplication.
   */
  EC& operator*=(const Number& scalar) {
    details::scalarMultiply<CURVE>(m_value, scalar);
    return *this;
  }

  /**
   * @brief Perform a scalar multiplication.
   */
  EC& operator*=(const ScalarField& scalar) {
    details::scalarMultiply<CURVE>(m_value, scalar);
    return *this;
  }

  /**
   * @brief Multiply a point with a scalar from the right.
   */
  friend EC operator*(const EC& point, const Number& scalar) {
    EC copy(point);
    return copy *= scalar;
  }

  /**
   * @brief Multiply a point with a scalar from the right.
   */
  friend EC operator*(const EC& point, const ScalarField& scalar) {
    EC copy(point);
    return copy *= scalar;
  }

  /**
   * @brief Multiply a point with a scalar from the left.
   */
  friend EC operator*(const Number& scalar, const EC& point) {
    return point * scalar;
  }

  /**
   * @brief Multiply a point with a scalar from the left.
   */
  friend EC operator*(const ScalarField& scalar, const EC& point) {
    return point * scalar;
  }

  /**
   * @brief Negate this point.
   */
  EC& negate() {
    details::negate<CURVE>(m_value);
    return *this;
  }

  /**
   * @brief Negate a curve point.
   */
  friend EC operator-(const EC& point) {
    EC tmp(point);
    return tmp.negate();
  }

  /**
   * @brief Check if this EC point is equal to another EC point.
   */
  bool equal(const EC& other) const {
    return details::equal<CURVE>(m_value, other.m_value);
  }

  /**
   * @brief Operator == for curve points.
   */
  friend bool operator==(const EC& lhs, const EC& rhs) {
    return lhs.equal(rhs);
  }

  /**
   * @brief Operator != for curve points.
   */
  friend bool operator!=(const EC& lhs, const EC& rhs) {
    return !(lhs == rhs);
  }

  /**
   * @brief Check if this point is equal to the point at inifity.
   */
  bool isPointAtInfinity() const {
    return details::isPointAtInfinity<CURVE>(m_value);
  }

  /**
   * @brief Return this point as a pair of affine coordinates.
   *
   * Only well-defined if the point is not the point at infinity.
   */
  std::array<Field, 2> toAffine() const {
    return details::toAffine<CURVE>(m_value);
  }

  /**
   * @brief Normalize this point.
   */
  void normalize() {
    if (isPointAtInfinity()) {
      details::setPointAtInfinity<CURVE>(m_value);
    } else {
      const auto afp = toAffine();
      details::setAffine<CURVE>(m_value, afp[0], afp[1]);
    }
  }

  /**
   * @brief Output this point as a string.
   */
  std::string toString() const {
    return details::toString<CURVE>(m_value);
  }

  /**
   * @brief Operator << for printing a curve point.
   */
  friend std::ostream& operator<<(std::ostream& os, const EC& e) {
    return os << e.toString();
  }

  /**
   * @brief Write this point to a buffer.
   */
  void write(unsigned char* dest, bool compress) const {
    details::toBytes<CURVE>(dest, m_value, compress);
  }

 private:
  typename CURVE::ValueType m_value;
};

/**
 * @brief Serializer for EC types.
 *
 * Elliptic curve points are serialized uncompressed and in affine form.
 */
template <typename CURVE>
struct Serializer<EC<CURVE>> {
  /**
   * @brief Get the size of a serialized EC point.
   */
  static constexpr std::size_t sizeOf(const EC<CURVE>& /* ignored */) {
    return EC<CURVE>::byteSize(false);
  }

  /**
   * @brief Write an EC point to a buffer.
   */
  static std::size_t write(const EC<CURVE>& point, unsigned char* buf) {
    point.write(buf, false);
    return sizeOf(point);
  }

  /**
   * @brief Read an EC point from a buffer.
   */
  static std::size_t read(EC<CURVE>& point, const unsigned char* buf) {
    point = EC<CURVE>::read(buf);
    return sizeOf(point);
  }
};

}  // namespace scl

#endif  // SCL_MATH_EC_H
