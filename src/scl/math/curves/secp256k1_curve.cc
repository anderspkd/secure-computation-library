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

#include <array>
#include <sstream>
#include <stdexcept>

#include "./../fields/secp256k1_helpers.h"
#include "scl/math/curves/ec_ops.h"
#include "scl/math/curves/secp256k1.h"
#include "scl/math/ec.h"

using namespace scl;

using Curve = math::ec::Secp256k1;
using Field = math::FF<Curve::Field>;
using Point = Curve::ValueType;

// clang-format off
#define POINT_AT_INFINITY Point{{Field{0}, Field{1}, Field{0}}}
// clang-format on

#define GET_X(point) (point)[0]
#define GET_Y(point) (point)[1]
#define GET_Z(point) (point)[2]

template <>
void math::ec::setPointAtInfinity<Curve>(Point& out) {
  out = POINT_AT_INFINITY;
}

namespace {

bool valid(const Field& x, const Field& y) {
  static const Field b(7);

  // valid if y^2 == x^3 + b
  auto lhs = y * y;
  auto rhs = x * x * x + b;
  return lhs == rhs;
}

}  // namespace

template <>
void math::ec::setAffine<Curve>(Point& out, const Field& x, const Field& y) {
  if (valid(x, y)) {
    out = {x, y, Field::one()};
  } else {
    throw std::invalid_argument("provided (x, y) not on curve");
  }
}

template <>
std::array<Field, 2> math::ec::toAffine<Curve>(const Point& point) {
  if (GET_Z(point) == Field::one()) {
    return {GET_X(point), GET_Y(point)};
  }
  const auto Z = GET_Z(point).inverse();
  return {GET_X(point) * Z, GET_Y(point) * Z};
}

template <>
bool math::ec::equal<Curve>(const Point& in1, const Point& in2) {
  const auto& Z1 = GET_Z(in1);
  const auto& Z2 = GET_Z(in2);
  // (X1, Y1, Z1) eqv (X2, Y2, Z2) <==> (X1 * Z2, Y1 * Z2) == (X2 * Z1, Y2 * Z2)
  return GET_X(in1) * Z2 == GET_X(in2) * Z1 &&
         GET_Y(in1) * Z2 == GET_Y(in2) * Z1;
}

template <>
bool math::ec::isPointAtInfinity<Curve>(const Point& point) {
  return GET_Z(point) == Field::zero();
}

template <>
std::string math::ec::toString<Curve>(const Point& point) {
  std::string str;
  if (isPointAtInfinity<Curve>(point)) {
    str = "EC{POINT_AT_INFINITY}";
  } else {
    auto ap = toAffine<Curve>(point);
    std::stringstream ss;
    ss << "EC{" << ap[0] << ", " << ap[1] << "}";
    str = ss.str();
  }
  return str;
}  // LCOV_EXCL_LINE

template <>
void math::ec::setGenerator<Curve>(Point& out) {
  static const Point gen = {
      Field::fromString(
          "79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798"),
      Field::fromString(
          "483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8"),
      Field{1}};

  out = gen;
}

namespace {

void addProj(Field& x1,
             Field& y1,
             Field& z1,
             const Field& x2,
             const Field& y2,
             const Field& z2) {
  static const Field b3(3 * 7);

  auto t0 = x1 * x2;
  auto t1 = y1 * y2;
  auto t2 = z1 * z2;

  auto t3 = x1 + y1;
  auto t4 = x2 + y2;
  t3 = t3 * t4;

  t4 = t0 + t1;
  t3 = t3 - t4;
  t4 = y1 + z1;

  auto x3 = y2 + z2;
  t4 = t4 * x3;
  x3 = t1 + t2;

  t4 = t4 - x3;
  x3 = x1 + z1;
  auto y3 = x2 + z2;

  x3 = x3 * y3;
  y3 = t0 + t2;
  y3 = x3 - y3;

  x3 = t0 + t0;
  t0 = x3 + t0;
  t2 = b3 * t2;

  auto z3 = t1 + t2;
  t1 = t1 - t2;
  y3 = b3 * y3;

  x3 = t4 * y3;
  t2 = t3 * t1;
  x3 = t2 - x3;

  y3 = y3 * t0;
  t1 = t1 * z3;
  y3 = t1 + y3;

  t0 = t0 * t3;
  z3 = z3 * t4;
  z3 = z3 + t0;

  x1 = x3;
  y1 = y3;
  z1 = z3;
}

void addMixed(Field& x1,
              Field& y1,
              Field& z1,
              const Field& x2,
              const Field& y2) {
  static const auto b3 = Field(3 * 7);

  auto t0 = x1 * x2;
  auto t1 = y1 * y2;
  auto t3 = x2 + y2;

  auto t4 = x1 + y1;
  t3 = t3 * t4;
  t4 = t0 + t1;

  t3 = t3 - t4;
  t4 = y2 * z1;
  t4 = t4 + y1;

  auto y3 = x2 * z1;
  y3 = y3 + x1;
  auto x3 = t0 + t0;

  t0 = x3 + t0;
  auto t2 = b3 * z1;
  auto z3 = t1 + t2;

  t1 = t1 - t2;
  y3 = b3 * y3;
  x3 = t4 * y3;

  t2 = t3 * t1;
  x3 = t2 - x3;
  y3 = y3 * t0;

  t1 = t1 * z3;
  y3 = t1 + y3;
  t0 = t0 * t3;

  z3 = z3 * t4;
  z3 = z3 + t0;

  x1 = x3;
  y1 = y3;
  z1 = z3;
}

}  // namespace

template <>
void math::ec::dbl<Curve>(Point& out) {
  // https://eprint.iacr.org/2015/1060.pdf algorithm 9.

  static const Field b3(3 * 7);

  auto t0 = GET_Y(out) * GET_Y(out);
  auto z3 = t0 + t0;
  z3 = z3 + z3;

  z3 = z3 + z3;
  auto t1 = GET_Y(out) * GET_Z(out);
  auto t2 = GET_Z(out) * GET_Z(out);

  t2 = b3 * t2;
  auto x3 = t2 * z3;
  auto y3 = t0 + t2;

  z3 = t1 * z3;
  t1 = t2 + t2;
  t2 = t1 + t2;

  t0 = t0 - t2;
  y3 = t0 * y3;
  y3 = x3 + y3;

  t1 = GET_X(out) * GET_Y(out);
  x3 = t0 * t1;
  x3 = x3 + x3;

  out[0] = x3;
  out[1] = y3;
  out[2] = z3;
}

template <>
void math::ec::add<Curve>(Point& out, const Point& in) {
  // https://eprint.iacr.org/2015/1060.pdf algorithm 7, 8

  if (GET_Z(in) == Field::one()) {
    addMixed(GET_X(out), GET_Y(out), GET_Z(out), GET_X(in), GET_Y(in));
  } else {
    addProj(GET_X(out),
            GET_Y(out),
            GET_Z(out),
            GET_X(in),
            GET_Y(in),
            GET_Z(in));
  }
}

template <>
void math::ec::negate<Curve>(Point& out) {
  if (GET_Y(out) == Field::zero()) {
    setPointAtInfinity<Curve>(out);
  } else {
    GET_Y(out).negate();
  }
}

template <>
void math::ec::subtract<Curve>(Point& out, const Point& in) {
  Point copy(in);
  ec::negate<Curve>(copy);
  ec::add<Curve>(out, copy);
}

template <>
void math::ec::scalarMultiply<Curve>(Point& out, const Number& scalar) {
  if (!isPointAtInfinity<Curve>(out)) {
    const auto n = scalar.bitSize();
    Point res;
    setPointAtInfinity<Curve>(res);
    // equivalent to for (int i = n - 1; i >= 0; i--)
    for (auto i = n; i-- > 0;) {
      dbl<Curve>(res);
      if (scalar.testBit(i)) {
        ec::add<Curve>(res, out);
      }
    }
    out = res;
  }
}

template <>
void math::ec::scalarMultiply<Curve>(Point& out,
                                     const FF<Curve::Scalar>& scalar) {
  if (!isPointAtInfinity<Curve>(out)) {
    Point res;
    setPointAtInfinity<Curve>(res);
    const auto naf = details::toNaf(scalar);
    for (auto i = naf.size; i-- > 0;) {
      ec::dbl<Curve>(res);
      if (naf.values[i].pos()) {
        ec::add<Curve>(res, out);
      } else if (naf.values[i].neg()) {
        ec::subtract<Curve>(res, out);
      }
    }
    out = res;
  }
}

// Flag indicating that the point was serialized as an (X, Y) pair
#define FULL_POINT_FLAG 0x04
// Flag indicating that the serialized point was the point at infinity. If the
// point was also serialized as a FULL_POINT, then we write the pair (0, 0).
#define POINT_AT_INFINITY_FLAG 0x02
// Flag indicating which of Y or -Y to select in case we serialize the point in
// compressed form.
#define SELECT_SMALLER_FLAG 0x01

#define IS_FULL_POINT(flags) ((flags)&FULL_POINT_FLAG)
#define IS_POINT_AT_INFINITY(flags) ((flags)&POINT_AT_INFINITY_FLAG)
#define SELECT_SMALLER(flags) ((flags)&SELECT_SMALLER_FLAG)

namespace {

Field computeOtherCoordinate(const Field& x) {
  static const Field CURVE_B(7);

  auto y_sqr = x * x * x + CURVE_B;
  auto z = math::details::sqrt(y_sqr);
  return z;
}

bool isSmaller(const Field& y, const Field& y_neg) {
  return math::details::isSmaller(y, y_neg);
}

}  // namespace

template <>
void math::ec::fromBytes<Curve>(Point& out, const unsigned char* src) {
  const auto flags = *src;

  if (IS_POINT_AT_INFINITY(flags)) {
    // we opt to not validate the rest of the buffer here. This technically
    // allows an implementation to only send a single byte in case it wishes to
    // send the point-at-infinity.
    setPointAtInfinity<Curve>(out);
  } else {
    if (IS_FULL_POINT(flags)) {
      out[0] = Field::read(src + 1);
      out[1] = Field::read(src + 1 + Field::byteSize());
      out[2] = Field::one();
    } else {
      Field x = Field::read(src + 1);

      out[0] = x;
      out[2] = Field::one();

      Field y = computeOtherCoordinate(x);
      Field yn = y.negated();

      auto smaller = isSmaller(y, yn);
      auto select_smaller = SELECT_SMALLER(flags);
      if (smaller) {
        out[1] = select_smaller == 0 ? yn : y;
      } else {
        out[1] = select_smaller == 0 ? y : yn;
      }
    }
  }
}

#define MARK_FULL_POINT(buf) (*(buf) |= FULL_POINT_FLAG)
#define MARK_POINT_AT_INFINITY(buf) (*(buf) |= POINT_AT_INFINITY_FLAG)
#define MARK_SELECT_SMALLER(buf) (*(buf) |= SELECT_SMALLER_FLAG)

template <>
void math::ec::toBytes<Curve>(unsigned char* dest,
                              const Point& in,
                              bool compress) {
  // Make sure flag byte is zeroed.
  *dest = 0;

  // if point is un-compressed, mark it as such.
  if (!compress) {
    MARK_FULL_POINT(dest);
  }

  if (isPointAtInfinity<Curve>(in)) {
    MARK_POINT_AT_INFINITY(dest);
    // zero rest of the buffer to ensure we can always safely send the right
    // amount of bytes.
    std::memset(dest + 1, 0, compress ? 32 : 64);
  } else {
    const auto ap = toAffine<Curve>(in);
    // if compression is used, we indicate a bit indicating which of {y, -y} is
    // the smaller, and the only write the x coordinate. Otherwise we write both
    // x and y.
    if (compress) {
      // include a flag which indicates which of {y, -y} is the smaller.
      const auto& y = ap[1];
      const auto yn = y.negated();

      if (isSmaller(y, yn)) {
        MARK_SELECT_SMALLER(dest);
      }
      ap[0].write(dest + 1);
    } else {
      ap[0].write(dest + 1);
      ap[1].write(dest + 1 + Field::byteSize());
    }
  }
}
