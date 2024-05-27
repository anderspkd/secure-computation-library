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

#ifndef SCL_TESTS_PROTOCOL_BEAVER_H
#define SCL_TESTS_PROTOCOL_BEAVER_H

#include <optional>

#include "./triple.h"
#include "scl/coro/task.h"
#include "scl/protocol/base.h"
#include "scl/protocol/env.h"
#include "scl/protocol/result.h"

namespace scl::test {

template <typename SHARE>
class BeaverMul final : public proto::Protocol {
 public:
  BeaverMul(SHARE x, SHARE y, Triple<SHARE> triple)
      : m_x(x), m_y(y), m_triple(triple) {}

  coro::Task<proto::ProtocolResult> run(proto::Env& env) const override {
    net::Packet packet;

    packet << m_x - m_triple.a;  // [e] = [x] - [a]
    packet << m_y - m_triple.b;  // [d] = [y] - [b]

    co_await env.network.party(0)->send(packet);
    co_await env.network.party(1)->send(packet);

    net::Packet packet0 = co_await env.network.party(0)->recv();
    net::Packet packet1 = co_await env.network.party(1)->recv();

    const auto e0 = packet0.read<SHARE>();
    const auto d0 = packet0.read<SHARE>();
    const auto e1 = packet1.read<SHARE>();
    const auto d1 = packet1.read<SHARE>();

    const auto e = e0 + e1;
    const auto d = d0 + d1;

    // [z] = ed + e[b] + d[a] + [c]. Only party 0 adds constants.
    auto z = e * m_triple.b + d * m_triple.a + m_triple.c;
    if (env.network.myId() == 0) {
      z += e * d;
    }

    co_return proto::ProtocolResult::done(z);
  }

 private:
  SHARE m_x;
  SHARE m_y;
  Triple<SHARE> m_triple;
};

}  // namespace scl::test

#endif  // SCL_TESTS_PROTOCOL_BEAVER_H
