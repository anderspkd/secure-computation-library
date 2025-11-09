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

#include <catch2/catch_test_macros.hpp>

#include "scl/protocol.h"
#include "scl/simulation/context.h"
#include "scl/simulation/network_description.h"

using namespace scl;
using namespace std::chrono_literals;

namespace {

class PingPongProtocol final : public Protocol {
 public:
  PingPongProtocol(bool is_sender) : m_is_sender(is_sender) {}

  Task<Result> run(Env& env) const override {
    if (m_is_sender) {
      Packet pkt;
      pkt << 123;
      // co_await 100ms;
      co_await env.network.other()->send(pkt);
    } else {
      auto pkt = co_await env.network.other()->recv();
    }

    co_return Result::done();
  }

 private:
  bool m_is_sender;
};

}  // namespace

TEST_CASE("Simulator test", "[sim]") {
  auto nd = NetworkDescription::createDefaultWAN(2);

  Simulator sim;
  auto res = sim.run(
      []() {
        std::vector<std::unique_ptr<Protocol>> p;
        p.emplace_back(std::make_unique<PingPongProtocol>(false));
        p.emplace_back(std::make_unique<PingPongProtocol>(true));
        return p;
      },
      nd);

  std::cout << "Party 0:\n";
  for (const auto& e : res[0]) {
    e->write(std::cout);
    std::cout << "\n";
  }

  std::cout << "\n\nParty 1:\n";
  for (const auto& e : res[1]) {
    e->write(std::cout);
    std::cout << "\n";
  }
}
