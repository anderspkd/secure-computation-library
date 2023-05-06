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

#include <catch2/catch.hpp>
#include <memory>

#include "scl/math/fp.h"
#include "scl/net/channel.h"
#include "scl/net/mem_channel.h"

using namespace scl;

struct DummyChannel final : public net::Channel {
  void Close() override {}

  void Send(const unsigned char* src, std::size_t n) override {
    (void)src;
    (void)n;
  }

  std::size_t Recv(unsigned char* dst, std::size_t n) override {
    (void)dst;
    recv++;
    return n;
  }

  bool HasData() override {
    has_data++;
    return there_is_data;
  }

  std::size_t recv = 0;
  std::size_t has_data = 0;
  bool there_is_data = false;
};

TEST_CASE("Channel non-block recv", "[net]") {
  std::unique_ptr<net::Channel> chl = std::make_unique<DummyChannel>();
  DummyChannel* dc = static_cast<DummyChannel*>(chl.get());

  auto p1 = chl->Recv(false);
  REQUIRE(!p1.has_value());
  REQUIRE(dc->has_data == 1);
  REQUIRE(dc->recv == 0);

  dc->there_is_data = true;
  auto p2 = chl->Recv(false);
  REQUIRE(p2.has_value());
  REQUIRE(dc->has_data == 2);
  REQUIRE(dc->recv == 2);
}
