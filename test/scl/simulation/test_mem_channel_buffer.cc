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

#include "scl/simulation/mem_channel_buffer.h"
#include "scl/util/time.h"

using namespace scl;

TEST_CASE("Simulation MemoryBackedChannelBuffer", "[sim]") {
  auto p = sim::MemoryBackedChannelBuffer::CreatePaired();

  auto chl0 = p[0];
  auto chl1 = p[1];

  REQUIRE(chl0->Size() == 0);
  REQUIRE(chl1->Size() == 0);

  std::vector<unsigned char> data = {1, 2, 3, 4};

  chl0->Write(data);

  REQUIRE(chl0->Size() == 0);
  REQUIRE(chl1->Size() == 4);

  auto d = chl1->Read(2);
  REQUIRE(d == std::vector<unsigned char>{1, 2});
  REQUIRE(chl1->Size() == 2);

  auto e = chl1->Read(2);
  REQUIRE(e == std::vector<unsigned char>{3, 4});
  REQUIRE(chl1->Size() == 0);
}

TEST_CASE("Simulation MemoryBackedChannelBuffer rollback", "[sim]") {
  // In the first two sections below, the prepare/rollback channel only ever
  // reads or writes. Never both.

  std::vector<unsigned char> data = {1, 2, 3, 4};

  SECTION("read/writes are correctly rolled back") {
    auto p = sim::MemoryBackedChannelBuffer::CreatePaired();
    auto local = p[0];
    auto remote = p[1];

    local->Prepare();

    local->Write(data);

    REQUIRE(remote->Size() == 4);

    local->Rollback();
    REQUIRE(remote->Size() == 0);

    remote->Write(data);

    local->Prepare();
    REQUIRE(local->Size() == 4);
    local->Read(2);
    REQUIRE(local->Size() == 2);
    local->Rollback();

    REQUIRE(local->Size() == 4);
  }

  SECTION("rollback only rolls back since last prepare") {
    auto p = sim::MemoryBackedChannelBuffer::CreatePaired();
    auto local = p[0];
    auto remote = p[1];

    local->Prepare();

    local->Write(data);
    local->Commit();

    local->Prepare();
    local->Write(data);

    REQUIRE(remote->Size() == 8);
    local->Rollback();

    REQUIRE(remote->Size() == 4);

    remote->Write(data);

    local->Prepare();

    REQUIRE(local->Size() == 4);
    local->Read(2);
    REQUIRE(local->Size() == 2);

    local->Rollback();
    REQUIRE(local->Size() == 4);
  }

  SECTION("rollback for loopback channel") {
    auto lo = sim::MemoryBackedChannelBuffer::CreateLoopback();

    lo->Prepare();
    lo->Write(data);
    REQUIRE(lo->Size() == 4);
    lo->Commit();

    lo->Prepare();

    lo->Write(data);
    REQUIRE(lo->Size() == 8);

    auto d3 = lo->Read(3);
    REQUIRE(d3 == std::vector<unsigned char>{1, 2, 3});
    REQUIRE(lo->Size() == 5);

    lo->Rollback();
    REQUIRE(lo->Size() == 4);
  }
}
