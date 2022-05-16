/**
 * @file test_simple.cc
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

#include <catch2/catch.hpp>

#include "scl/math.h"
#include "scl/networking.h"
#include "scl/p/simple.h"
#include "scl/secret_sharing.h"

using FF = scl::FF<61>;

struct Triple {
  Triple(FF a, FF b, FF c) : a(a), b(b), c(c){};

  FF a;
  FF b;
  FF c;
};

struct Context {
  unsigned id;
  scl::Network network;
};

class BeaverMulFinalize
    : public scl::LastProtocolStep<BeaverMulFinalize, Context> {
 public:
  BeaverMulFinalize(Triple t) : mTriple(t){};

  FF Finalize(Context& ctx) {
    scl::Vec<FF> ed0(2);
    scl::Vec<FF> ed1(2);
    ctx.network.Party(0)->Recv(ed0);
    ctx.network.Party(1)->Recv(ed1);

    auto e = ed0[0] + ed1[0];
    auto d = ed0[1] + ed1[1];

    if (ctx.id == 0) {
      return e * d - e * mTriple.b - d * mTriple.a + mTriple.c;
    } else {
      return -e * mTriple.b - d * mTriple.a + mTriple.c;
    }
  };

 private:
  Triple mTriple;
};

class BeaverMul : public scl::ProtocolStep<BeaverMul, Context> {
 public:
  BeaverMul(Triple t, FF x, FF y) : mTriple(t), x(x), y(y){};

  BeaverMulFinalize Run(Context& ctx) {
    auto e = x + mTriple.a;
    auto d = y + mTriple.b;

    ctx.network.Party(0)->Send(scl::Vec<FF>{e, d});
    ctx.network.Party(1)->Send(scl::Vec<FF>{e, d});

    return BeaverMulFinalize(mTriple);
  };

 private:
  Triple mTriple;
  FF x;
  FF y;
};

static inline std::vector<Triple> RandomTriple() {
  scl::PRG prg;
  auto a = FF::Random(prg);
  auto b = FF::Random(prg);
  auto c = a * b;

  auto as = scl::CreateAdditiveShares(a, 2, prg);
  auto bs = scl::CreateAdditiveShares(b, 2, prg);
  auto cs = scl::CreateAdditiveShares(c, 2, prg);

  return std::vector<Triple>{{as[0], bs[0], cs[0]}, {as[1], bs[1], cs[1]}};
}

TEST_CASE("protocol") {
  scl::PRG prg;
  auto xs = scl::CreateAdditiveShares(FF(42), 2, prg);
  auto ys = scl::CreateAdditiveShares(FF(11), 2, prg);
  auto ts = RandomTriple();
  auto networks = scl::Network::CreateFullInMemory(2);

  Context ctx0{0, networks[0]};
  Context ctx1{1, networks[1]};

  BeaverMul m0(ts[0], xs[0], ys[0]);
  BeaverMul m1(ts[1], xs[1], ys[1]);

  auto f0 = m0.Run(ctx0);
  auto f1 = m1.Run(ctx1);

  auto z0 = f0.Finalize(ctx0);
  auto z1 = f1.Finalize(ctx1);

  REQUIRE(z0 + z1 == FF(42) * FF(11));
}
