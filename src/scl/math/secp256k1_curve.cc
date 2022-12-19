/**
 * @file secp256k1_curve.cc
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

#include <array>
#include <sstream>
#include <stdexcept>

#include "./secp256k1_extras.h"
#include "scl/math/curves/secp256k1.h"
#include "scl/math/ec.h"
#include "scl/math/ec_ops.h"
#include "scl/math/fp.h"

using Curve = scl::details::Secp256k1;
using Field = scl::FF<Curve::Field>;
using Point = Curve::ValueType;

// clang-format off
#define POINT_AT_INFINITY Point{{Field{0}, Field{1}, Field{0}}}
// clang-format on

#define GET_X(point) (point)[0]
#define GET_Y(point) (point)[1]
#define GET_Z(point) (point)[2]

static const Field kCurveB(7);

template <>
void scl::details::CurveSetPointAtInfinity<Curve>(Point& out) {
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
void scl::details::CurveSetAffine<Curve>(Point& out,
                                         const Field& x,
                                         const Field& y) {
  if (Valid(x, y)) {
    out = {x, y, Field(1)};
  } else {
    throw std::invalid_argument("provided (x, y) not on curve");
  }
}

template <>
std::array<Field, 2> scl::details::CurveToAffine<Curve>(const Point& point) {
  const auto Z = GET_Z(point);
  return {GET_X(point) / Z, GET_Y(point) / Z};
}

template <>
bool scl::details::CurveEqual<Curve>(const Point& in1, const Point& in2) {
  const auto Z1 = GET_Z(in1);
  const auto Z2 = GET_Z(in2);
  // (X1, Y1, Z1) eqv (X2, Y2, Z2) <==> (X1 * Z2, Y1 * Z2) == (X2 * Z1, Y2 * Z2)
  return GET_X(in1) * Z2 == GET_X(in2) * Z1 &&
         GET_Y(in1) * Z2 == GET_Y(in2) * Z1;
}

template <>
bool scl::details::CurveIsPointAtInfinity<Curve>(const Point& point) {
  return CurveEqual<Curve>(point, POINT_AT_INFINITY);
}

template <>
std::string scl::details::CurveToString<Curve>(const Point& point) {
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
}

template <>
void scl::details::CurveSetGenerator<Curve>(Point& out) {
  static const Point gen = {
      Field::FromString(
          "79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798"),
      Field::FromString(
          "483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8"),
      Field{1}};

  out = gen;
}

template <>
void scl::details::CurveDouble<Curve>(Point& out) {
  if (!CurveIsPointAtInfinity<Curve>(out)) {
    if (GET_Y(out) == Field::Zero()) {
      CurveSetPointAtInfinity<Curve>(out);
    } else if (!CurveIsPointAtInfinity<Curve>(out)) {
      const auto X = GET_X(out);
      const auto Y = GET_Y(out);
      const auto Z = GET_Z(out);

      const auto W = Field(3) * X * X;
      const auto S = Y * Z;
      const auto B = X * Y * S;
      const auto eight = Field(8);
      const auto H = W * W - eight * B;

      out[0] = Field(2) * H * S;
      const auto Ssqr = S * S;
      out[1] = W * (Field(4) * B - H) - eight * Y * Y * Ssqr;
      out[2] = eight * Ssqr * S;
    }
  }
}

template <>
void scl::details::CurveAdd<Curve>(Point& out, const Point& in) {
  if (CurveIsPointAtInfinity<Curve>(out)) {
    out = in;
  } else if (!CurveIsPointAtInfinity<Curve>(in)) {
    const auto X1 = GET_X(out);
    const auto Y1 = GET_Y(out);
    const auto Z1 = GET_Z(out);
    const auto X2 = GET_X(in);
    const auto Y2 = GET_Y(in);
    const auto Z2 = GET_Z(in);

    const auto U1 = Y2 * Z1;
    const auto U2 = Y1 * Z2;
    const auto V1 = X2 * Z1;
    const auto V2 = X1 * Z2;

    if (V1 == V2) {
      if (U1 != U2) {
        CurveSetPointAtInfinity<Curve>(out);
      } else {
        CurveDouble<Curve>(out);
      }
    } else {
      const auto U = U1 - U2;
      const auto V = V1 - V2;
      const auto W = Z1 * Z2;
      const auto Vsqr = V * V;
      const auto VsqrV2 = Vsqr * V2;
      const auto Vcbe = Vsqr * V;
      const auto A = U * U * W - Vcbe - Field(2) * VsqrV2;
      out[0] = V * A;
      out[1] = U * (VsqrV2 - A) - Vcbe * U2;
      out[2] = Vcbe * W;
    }
  }
}

template <>
void scl::details::CurveNegate<Curve>(Point& out) {
  if (GET_Y(out) == Field::Zero()) {
    CurveSetPointAtInfinity<Curve>(out);
  } else {
    GET_Y(out).Negate();
  }
}

template <>
void scl::details::CurveSubtract<Curve>(Point& out, const Point& in) {
  Point copy(in);
  CurveNegate<Curve>(copy);
  CurveAdd<Curve>(out, copy);
}

template <>
void scl::details::CurveScalarMultiply<Curve>(Point& out,
                                              const Number& scalar) {
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
void scl::details::CurveScalarMultiply<Curve>(Point& out,
                                              const FF<Curve::Order>& scalar) {
  if (!CurveIsPointAtInfinity<Curve>(out)) {
    auto x = scl::SCL_FF_Extras<Curve::Order>::FromMonty(scalar);
    const auto n = scl::SCL_FF_Extras<Curve::Order>::HigestSetBit(x);
    Point res;
    CurveSetPointAtInfinity<Curve>(res);
    for (auto i = n; i-- > 0;) {
      CurveDouble<Curve>(res);
      if (scl::SCL_FF_Extras<Curve::Order>::TestBit(x, i)) {
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
  auto z = scl::SCL_FF_Extras<Curve::Field>::ComputeSqrt(y_sqr);
  return z;
}

bool IsSmaller(const Field& y, const Field& y_neg) {
  return scl::SCL_FF_Extras<Curve::Field>::IsSmaller(y, y_neg);
}

}  // namespace

template <>
void scl::details::CurveFromBytes<Curve>(Point& out, const unsigned char* src) {
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
void scl::details::CurveToBytes<Curve>(unsigned char* dest,
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
      const auto y = ap[1];
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
