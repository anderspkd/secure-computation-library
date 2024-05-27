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

#ifndef SCL_PROTOCOL_BASE_H
#define SCL_PROTOCOL_BASE_H

#include <any>
#include <chrono>
#include <memory>
#include <thread>

#include "scl/net/network.h"
#include "scl/protocol/env.h"
#include "scl/protocol/result.h"

namespace scl::proto {

/**
 * @brief Interface for protocols.
 *
 * A class implementing this interface defines the code that a party runs in an
 * interactive protocol. An example is given below, showing how we might
 * implement a classical secure multiplication protocol using a multiplication
 * triple.
 *
 * @code
 * template <typename SHARE>
 * class BeaverMul final : public proto::Protocol {
 *  public:
 *   BeaverMul(SHARE x, SHARE y, Triple<SHARE> triple)
 *       : m_x(x), m_y(y), m_triple(triple) {}
 *
 *   coro::Task<proto::ProtocolResult> run(proto::Env& env) const override {
 *     net::Packet packet;
 *
 *     packet << m_x - m_triple.a;  // [e] = [x] - [a]
 *     packet << m_y - m_triple.b;  // [d] = [y] - [b]
 *
 *     co_await env.network.party(0)->send(packet);
 *     co_await env.network.party(1)->send(packet);
 *
 *     net::Packet packet0 = co_await env.network.party(0)->recv();
 *     net::Packet packet1 = co_await env.network.party(1)->recv();
 *
 *     const SHARE e0 = packet0.read<SHARE>();
 *     const SHARE d0 = packet0.read<SHARE>();
 *     const SHARE e1 = packet1.read<SHARE>();
 *     const SHARE d1 = packet1.read<SHARE>();
 *
 *     const SHARE e = e0 + e1;
 *     const SHARE d = d0 + d1;
 *
 *     // [z] = ed + e[b] + d[a] + [c]. Only party 0 adds constants.
 *     SHARE z = e * m_triple.b + d * m_triple.a + m_triple.c;
 *     if (env.network.myId() == 0) {
 *       z += e * d;
 *     }
 *
 *     co_return proto::ProtocolResult::done(z);
 *   }
 *
 *  private:
 *   SHARE m_x;
 *   SHARE m_y;
 *   Triple<SHARE> m_triple;
 * };
 * @endcode
 *
 * It is possible to chain multiple protocols together by returning a pointer to
 * the next protocol. It is also possible to compose protocol objects, e.g., as
 * shown below. However, care has to be taken in handling cases where two
 * protocols both attempt to read from the same channel.
 *
 * @code
 * struct SimpleProtocol final : public proto::Protocol {
 *   coro::Task<proto::ProtocolResult> run(proto::Env& env) const override {
 *     // ... do stuff
 *     co_return proto::ProtocolResult::done(some_value);
 *   }
 * };
 *
 * class Composed final : public proto::Protocol {
 *  public:
 *   Composed(SimpleProtocol&& protocol1, SimpleProtocol&& protocol2)
 *     : m_protocol1(std::move(protocol1)), m_protocol2(std::move(protocol2)) {}
 *
 *   coro::Task<proto::ProtocolResult> run(proto::Env& env) const override {
 *     // batch the two protocols.
 *     std::vector<coro::Task<proto::ProtocolResult>> protocols;
 *     protocols.emplace_back(m_protocol1.run());
 *     protocols.emplace_back(m_protocol2.run());
 *
 *     // ask the coroutine runtime to run them in any random order.
 *     std::vector<proto::ProtocolResult> results =
 *       co_await coro::batch(std::move(protocols));
 *
 *     co_return results;
 *   }
 *
 *  private:
 *   SimpleProtocol m_protocol1;
 *   SimpleProtocol m_protocol2;
 * };
 * @endcode
 *
 * Each Protocol is associated with a name, defaulting to the value of
 * Protocol::DEFAULT_NAME. The name is used only in the simulator to group
 * measurements when generating a result.
 */
struct Protocol {
  virtual ~Protocol() {}

  /**
   * @brief Default protocol name.
   */
  constexpr static const char* DEFAULT_NAME = "UNNAMED";

  /**
   * @brief Run the protocol.
   */
  virtual coro::Task<ProtocolResult> run(Env& env) const = 0;

  /**
   * @brief The protocol's name.
   */
  virtual std::string name() const {
    return Protocol::DEFAULT_NAME;
  }
};

}  // namespace scl::proto

#endif  // SCL_PROTOCOL_BASE_H
