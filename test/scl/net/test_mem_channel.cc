/**
 * @file test_mem_channel.cc
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
#include <cstring>
#include <iostream>
#include <vector>

#include "scl/math.h"
#include "scl/net/mem_channel.h"
#include "scl/primitives/prg.h"
#include "util.h"

void PrintBuf(const unsigned char* b, std::size_t n) {
  for (std::size_t i = 0; i < n; ++i) {
    std::cout << (int)b[i] << " ";
  }
  std::cout << ""
            << "\n";
}

TEST_CASE("InMemoryChannel", "[network]") {
  auto channels = scl::InMemoryChannel::CreatePaired();
  auto chl0 = channels[0];
  auto chl1 = channels[1];

  auto prg = scl::PRG::Create();
  unsigned char data_in[200] = {0};
  prg.Next(data_in, 200);

  SECTION("Send and receive") {
    unsigned char data_out[200] = {0};
    REQUIRE(!chl1->HasData());
    chl0->Send(data_in, 200);
    REQUIRE(!chl0->HasData());
    REQUIRE(chl1->HasData());
    chl1->Recv(data_out, 200);
    REQUIRE(scl_tests::BufferEquals(data_in, data_out, 200));
  }

  chl0->Flush();
  chl1->Flush();

  SECTION("Send chunked") {
    unsigned char data_out[200] = {0};

    chl0->Send(data_in, 50);
    chl0->Send(data_in + 50, 50);
    chl0->Send(data_in + 100, 100);
    chl1->Recv(data_out, 200);

    REQUIRE(scl_tests::BufferEquals(data_in, data_out, 200));
  }

  chl0->Flush();
  chl1->Flush();

  SECTION("Recv chunked") {
    unsigned char data_out[200] = {0};
    chl0->Send(data_in, 100);
    chl0->Send(data_in + 100, 100);
    chl1->Recv(data_out, 100);
    chl1->Recv(data_out + 100, 100);

    REQUIRE(scl_tests::BufferEquals(data_in, data_out, 200));
  }

  chl0->Flush();
  chl1->Flush();

  SECTION("Send trivial data") {
    scl::Channel* c0 = chl0.get();
    scl::Channel* c1 = chl1.get();
    int x = 123;
    c0->Send(x);
    int y;
    c1->Recv(y);
    REQUIRE(x == y);
  }

  chl0->Flush();
  chl1->Flush();

  SECTION("Send vector trivial data") {
    scl::Channel* c0 = chl0.get();
    scl::Channel* c1 = chl1.get();
    std::vector<long> data = {1, 2, 3, 4, 11111111};
    c0->Send(data);
    std::vector<long> recv;
    c1->Recv(recv);
    REQUIRE(data == recv);
    REQUIRE(recv.size() == data.size());
  }

  using FF = scl::Fp<61>;
  using Vec = scl::Vec<FF>;

  chl0->Flush();
  chl1->Flush();

  SECTION("Send Vec") {
    scl::Channel* c0 = chl0.get();
    scl::Channel* c1 = chl1.get();
    Vec v = {FF(1), FF(5), FF(2) - FF(10)};
    c0->Send(v);
    Vec w;
    c1->Recv(w);
    REQUIRE(v.Equals(w));
  }

  chl0->Flush();
  chl1->Flush();

  using Mat = scl::Mat<FF>;

  SECTION("Send mat") {
    scl::Channel* c0 = chl0.get();
    scl::Channel* c1 = chl1.get();
    auto m = Mat::Random(5, 7, prg);
    c0->Send(m);
    Mat a;
    c1->Recv(a);
    REQUIRE(m.Equals(a));
  }

  SECTION("Send self") {
    auto c = scl::InMemoryChannel::CreateSelfConnecting();

    c->Send(data_in, 20);
    c->Send(data_in + 20, 100);
    c->Send(data_in + 120, 80);

    unsigned char data_out[200] = {0};
    c->Recv(data_out, 10);
    c->Recv(data_out + 10, 100);
    c->Recv(data_out + 110, 90);

    REQUIRE(scl_tests::BufferEquals(data_in, data_out, 200));
  }
}
