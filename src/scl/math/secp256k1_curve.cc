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

#include <array>
#include <sstream>
#include <stdexcept>

#include "./secp256k1_helpers.h"
#include "scl/math/curves/secp256k1.h"
#include "scl/math/ec.h"
#include "scl/math/ec_ops.h"
#include "scl/math/fp.h"

using namespace scl;

using Curve = math::Secp256k1;
using Field = math::FF<Curve::Field>;
using Point = Curve::ValueType;

// clang-format off
#define POINT_AT_INFINITY Point{{Field{0}, Field{1}, Field{0}}}
// clang-format on

#define GET_X(point) (point)[0]
#define GET_Y(point) (point)[1]
#define GET_Z(point) (point)[2]

static const Field kCurveB(7);

template <>
void math::CurveSetPointAtInfinity<Curve>(Point& out) {
  out = POINT_AT_INFINITY;
}

namespace {

bool Valid(const Field& x, const Field& y) {
  auto lhs = y * y;
  auto rhs = x * x * x + kCurveB;
  return lhs == rhs;
}

}  // namespace

template <>
void math::CurveSetAffine<Curve>(Point& out, const Field& x, const Field& y) {
  if (Valid(x, y)) {
    out = {x, y, Field::One()};
  } else {
    throw std::invalid_argument("provided (x, y) not on curve");
  }
}

template <>
std::array<Field, 2> math::CurveToAffine<Curve>(const Point& point) {
  const auto Z = GET_Z(point).Inverse();
  return {GET_X(point) * Z, GET_Y(point) * Z};
}

template <>
bool math::CurveEqual<Curve>(const Point& in1, const Point& in2) {
  const auto& Z1 = GET_Z(in1);
  const auto& Z2 = GET_Z(in2);
  // (X1, Y1, Z1) eqv (X2, Y2, Z2) <==> (X1 * Z2, Y1 * Z2) == (X2 * Z1, Y2 * Z2)
  return GET_X(in1) * Z2 == GET_X(in2) * Z1 &&
         GET_Y(in1) * Z2 == GET_Y(in2) * Z1;
}

template <>
bool math::CurveIsPointAtInfinity<Curve>(const Point& point) {
  return GET_Z(point) == Field::Zero();
}

template <>
std::string math::CurveToString<Curve>(const Point& point) {
  std::string str;
  if (CurveIsPointAtInfinity<Curve>(point)) {
    str = "EC{POINT_AT_INFINITY}";
  } else {
    auto ap = CurveToAffine<Curve>(point);
    std::stringstream ss;
    ss << "EC{" << ap[0] << ", " << ap[1] << "}";
    str = ss.str();
  }
  return str;
}  // LCOV_EXCL_LINE

template <>
void math::CurveSetGenerator<Curve>(Point& out) {
  static const Point gen = {
      Field::FromString(
          "79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798"),
      Field::FromString(
          "483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8"),
      Field{1}};

  out = gen;
}

template <>
void math::CurveDouble<Curve>(Point& out) {
  // https://eprint.iacr.org/2015/1060.pdf algorithm 9.

  static const auto b3 = Field(3 * 7);

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
void math::CurveAdd<Curve>(Point& out, const Point& in) {
  // https://eprint.iacr.org/2015/1060.pdf algorithm 7

  static const auto b3 = Field(3 * 7);

  auto t0 = GET_X(out) * GET_X(in);
  auto t1 = GET_Y(out) * GET_Y(in);
  auto t2 = GET_Z(out) * GET_Z(in);

  auto t3 = GET_X(out) + GET_Y(out);
  auto t4 = GET_X(in) + GET_Y(in);
  t3 = t3 * t4;

  t4 = t0 + t1;
  t3 = t3 - t4;
  t4 = GET_Y(out) + GET_Z(out);

  auto x3 = GET_Y(in) + GET_Z(in);
  t4 = t4 * x3;
  x3 = t1 + t2;

  t4 = t4 - x3;
  x3 = GET_X(out) + GET_Z(out);
  auto y3 = GET_X(in) + GET_Z(in);

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

  out[0] = x3;
  out[1] = y3;
  out[2] = z3;
}

template <>
void math::CurveNegate<Curve>(Point& out) {
  if (GET_Y(out) == Field::Zero()) {
    CurveSetPointAtInfinity<Curve>(out);
  } else {
    GET_Y(out).Negate();
  }
}

template <>
void math::CurveSubtract<Curve>(Point& out, const Point& in) {
  Point copy(in);
  CurveNegate<Curve>(copy);
  CurveAdd<Curve>(out, copy);
}

template <>
void math::CurveScalarMultiply<Curve>(Point& out, const Number& scalar) {
  if (!CurveIsPointAtInfinity<Curve>(out)) {
    const auto n = scalar.BitSize();
    Point res;
    CurveSetPointAtInfinity<Curve>(res);
    // equivalent to for (int i = n - 1; i >= 0; i--)
    for (auto i = n; i-- > 0;) {
      CurveDouble<Curve>(res);
      if (scalar.TestBit(i)) {
        CurveAdd<Curve>(res, out);
      }
    }
    out = res;
  }
}

template <>
void math::CurveScalarMultiply<Curve>(Point& out,
                                      const FF<Curve::Scalar>& scalar) {
  if (!CurveIsPointAtInfinity<Curve>(out)) {
    auto x = FFAccess<Curve::Scalar>::FromMonty(scalar);
    const auto n = FFAccess<Curve::Scalar>::HigestSetBit(x);
    Point res;
    CurveSetPointAtInfinity<Curve>(res);
    for (auto i = n; i-- > 0;) {
      CurveDouble<Curve>(res);
      if (FFAccess<Curve::Scalar>::TestBit(x, i)) {
        CurveAdd<Curve>(res, out);
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

Field ComputeOtherCoordinate(const Field& x) {
  auto y_sqr = x * x * x + kCurveB;
  auto z = math::FFAccess<Curve::Field>::ComputeSqrt(y_sqr);
  return z;
}

bool IsSmaller(const Field& y, const Field& y_neg) {
  return math::FFAccess<Curve::Field>::IsSmaller(y, y_neg);
}

}  // namespace

template <>
void math::CurveFromBytes<Curve>(Point& out, const unsigned char* src) {
  const auto flags = *src;

  if (IS_POINT_AT_INFINITY(flags)) {
    // we opt to not validate the rest of the buffer here. This technically
    // allows an implementation to only send a single byte in case it wishes to
    // send the point-at-infinity.
    CurveSetPointAtInfinity<Curve>(out);
  } else {
    if (IS_FULL_POINT(flags)) {
      out[0] = Field::Read(src + 1);
      out[1] = Field::Read(src + 1 + Field::ByteSize());
      out[2] = Field::One();
    } else {
      Field x = Field::Read(src + 1);

      out[0] = x;
      out[2] = Field::One();

      Field y = ComputeOtherCoordinate(x);
      Field yn = y.Negated();

      auto smaller = IsSmaller(y, yn);
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
void math::CurveToBytes<Curve>(unsigned char* dest,
                               const Point& in,
                               bool compress) {
  // Make sure flag byte is zeroed.
  *dest = 0;

  // if point is un-compressed, mark it as such.
  if (!compress) {
    MARK_FULL_POINT(dest);
  }

  if (CurveIsPointAtInfinity<Curve>(in)) {
    MARK_POINT_AT_INFINITY(dest);
    // zero rest of the buffer to ensure we can always safely send the right
    // amount of bytes.
    std::memset(dest + 1, 0, compress ? 32 : 64);
  } else {
    const auto ap = CurveToAffine<Curve>(in);
    // if compression is used, we indicate a bit indicating which of {y, -y} is
    // the smaller, and the only write the x coordinate. Otherwise we write both
    // x and y.
    if (compress) {
      // include a flag which indicates which of {y, -y} is the smaller.
      const auto& y = ap[1];
      const auto yn = y.Negated();

      if (IsSmaller(y, yn)) {
        MARK_SELECT_SMALLER(dest);
      }
      ap[0].Write(dest + 1);
    } else {
      ap[0].Write(dest + 1);
      ap[1].Write(dest + 1 + Field::ByteSize());
    }
  }
}
