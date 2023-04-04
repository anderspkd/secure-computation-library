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
#include <cstring>
#include <iostream>
#include <vector>

#include "scl/math/fp.h"
#include "scl/math/vec.h"
#include "scl/net/mem_channel.h"
#include "scl/util/prg.h"
#include "util.h"

using namespace scl;

TEST_CASE("MemoryBackedChannel close", "[net]") {
  auto channel = net::MemoryBackedChannel::CreateLoopback();
  channel->Close();
}

TEST_CASE("MemoryBackedChannel send/recv", "[net]") {
  auto channels = net::MemoryBackedChannel::CreatePaired();
  auto chl0 = channels[0];
  auto chl1 = channels[1];

  auto prg = util::PRG::Create();
  unsigned char data_in[200] = {0};
  prg.Next(data_in, 200);

  unsigned char data_out[200] = {0};
  REQUIRE(!chl1->HasData());
  chl0->Send(data_in, 200);
  REQUIRE(!chl0->HasData());
  REQUIRE(chl1->HasData());
  chl1->Recv(data_out, 200);
  REQUIRE(test::BufferEquals(data_in, data_out, 200));
}

TEST_CASE("MemoryBackedChannel send chunked", "[net]") {
  auto channels = net::MemoryBackedChannel::CreatePaired();
  auto chl0 = channels[0];
  auto chl1 = channels[1];

  auto prg = util::PRG::Create();
  unsigned char data_in[200] = {0};
  unsigned char data_out[200] = {0};

  prg.Next(data_in, 200);

  chl0->Send(data_in, 50);
  chl0->Send(data_in + 50, 50);
  chl0->Send(data_in + 100, 100);
  chl1->Recv(data_out, 200);

  REQUIRE(test::BufferEquals(data_in, data_out, 200));
}

TEST_CASE("MemoryBackedChannel recv chunked", "[net]") {
  auto channels = net::MemoryBackedChannel::CreatePaired();
  auto chl0 = channels[0];
  auto chl1 = channels[1];

  auto prg = util::PRG::Create();
  unsigned char data_in[200] = {0};
  unsigned char data_out[200] = {0};

  prg.Next(data_in, 200);

  chl0->Send(data_in, 100);
  chl0->Send(data_in + 100, 100);
  chl1->Recv(data_out, 100);
  chl1->Recv(data_out + 100, 100);

  REQUIRE(test::BufferEquals(data_in, data_out, 200));
}

TEST_CASE("MemoryBackedChannel trivial data", "[net]") {
  auto channels = net::MemoryBackedChannel::CreatePaired();
  auto chl0 = channels[0];
  auto chl1 = channels[1];

  net::Channel* c0 = chl0.get();
  net::Channel* c1 = chl1.get();
  int x = 123;
  c0->Send(x);
  int y;
  c1->Recv(y);
  REQUIRE(x == y);
}

TEST_CASE("MemoryBackedChannel std::vector", "[net]") {
  auto channels = net::MemoryBackedChannel::CreatePaired();
  auto chl0 = channels[0];
  auto chl1 = channels[1];

  net::Channel* c0 = chl0.get();
  net::Channel* c1 = chl1.get();
  std::vector<long> data = {1, 2, 3, 4, 11111111};
  c0->Send(data);
  std::vector<long> recv;
  c1->Recv(recv);
  REQUIRE(data == recv);
  REQUIRE(recv.size() == data.size());
}

TEST_CASE("MemoryBackedChannel Vec", "[net]") {
  using FF = math::Fp<61>;
  auto channels = net::MemoryBackedChannel::CreatePaired();
  auto chl0 = channels[0];
  auto chl1 = channels[1];

  net::Channel* c0 = chl0.get();
  net::Channel* c1 = chl1.get();
  math::Vec<FF> v = {FF(1), FF(5), FF(2) - FF(10)};
  c0->Send(v);
  math::Vec<FF> w;
  c1->Recv(w);
  REQUIRE(v.Equals(w));
}

TEST_CASE("MemoryBackedChannel Mat", "[net]") {
  using FF = math::Fp<61>;
  auto channels = net::MemoryBackedChannel::CreatePaired();
  auto chl0 = channels[0];
  auto chl1 = channels[1];
  auto prg = util::PRG::Create("MemoryBackedChannel Mat");

  net::Channel* c0 = chl0.get();
  net::Channel* c1 = chl1.get();
  auto m = math::Mat<FF>::Random(5, 7, prg);
  c0->Send(m);
  math::Mat<FF> a;
  c1->Recv(a);
  REQUIRE(m.Equals(a));
}

TEST_CASE("MemoryBackedChannel send to self", "[net]") {
  auto c = net::MemoryBackedChannel::CreateLoopback();
  unsigned char data_in[200] = {0};

  c->Send(data_in, 20);
  c->Send(data_in + 20, 100);
  c->Send(data_in + 120, 80);

  unsigned char data_out[200] = {0};
  c->Recv(data_out, 10);
  c->Recv(data_out + 10, 100);
  c->Recv(data_out + 110, 90);

  REQUIRE(test::BufferEquals(data_in, data_out, 200));
}
