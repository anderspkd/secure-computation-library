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

#ifndef SCL_TESTS_PROTOCOL_BEAVER_H
#define SCL_TESTS_PROTOCOL_BEAVER_H

#include <optional>

#include "./triple.h"
#include "scl/protocol/base.h"
#include "scl/protocol/env.h"

namespace scl::test {

template <typename FF>
struct BeaverMul {
  class Init;
  class Finalize;

  static std::unique_ptr<proto::Protocol> Create(FF x,
                                                 FF y,
                                                 Triple<FF> triple) {
    return std::make_unique<Init>(x, y, triple);
  }
};

template <typename FF>
class BeaverMul<FF>::Init final : public proto::Protocol {
 public:
  Init(FF x, FF y, Triple<FF> triple) : x(x), y(y), mTriple(triple){};

  std::unique_ptr<proto::Protocol> Run(
      proto::ProtocolEnvironment& env) override {
    math::Vec<FF> elems(2 * 100);

    for (std::size_t i = 0; i < 200; i += 2) {
      auto e = x + mTriple.a;
      auto d = y + mTriple.b;
      elems[i] = e;
      elems[i + 1] = d;
    }

    env.network.Party(0)->Send(elems);
    env.network.Party(1)->Send(elems);

    return std::make_unique<Finalize>(mTriple);
  }

  std::string Name() const override {
    return "init";
  };

 private:
  FF x;
  FF y;
  Triple<FF> mTriple;
};

template <typename FF>
class BeaverMul<FF>::Finalize final : public proto::Protocol {
 public:
  Finalize(Triple<FF> triple) : mTriple(triple){};

  std::unique_ptr<Protocol> Run(proto::ProtocolEnvironment& env) override {
    math::Vec<FF> ed0(2 * 100);
    math::Vec<FF> ed1(2 * 100);

    env.network.Party(0)->Recv(ed0);
    env.network.Party(1)->Recv(ed1);

    math::Vec<FF> output(100);

    std::size_t output_idx = 0;
    for (std::size_t i = 0; i < 200; i += 2) {
      auto e = ed0[i] + ed1[i];
      auto d = ed0[i + 1] + ed1[i + 1];

      // constant addition
      if (env.network.MyId() == 0) {
        output[output_idx++] =
            e * d - e * mTriple.b - d * mTriple.a + mTriple.c;
      } else {
        output[output_idx++] = -e * mTriple.b - d * mTriple.a + mTriple.c;
      }
    }

    mOutput = output;

    return nullptr;
  };

  std::string Name() const override {
    return "finalize";
  };

  std::any Output() const override {
    return mOutput;
  };

 private:
  Triple<FF> mTriple;
  std::any mOutput;
};

}  // namespace scl::test

#endif  // SCL_TESTS_PROTOCOL_BEAVER_H
