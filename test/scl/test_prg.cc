/**
 * @file test_prg.cc
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

#include "scl/prg.h"

inline bool BufferCmp(const unsigned char* b0, const unsigned char* b1,
                      unsigned len) {
  auto p0 = b0;
  auto p1 = b1;
  while (len-- > 0)
    if (*p0++ != *p1++) return false;
  return true;
}

inline bool BufferLooksRandom(const unsigned char* p, unsigned len) {
  std::vector<unsigned> buckets(256);

  for (std::size_t i = 0; i < len; i++) {
    buckets[p[i]]++;
  }

  bool all_in_interval = true;
  for (std::size_t i = 0; i < 256; i++) {
    auto p = 100 * ((float)buckets[i] / len);
    all_in_interval &= p >= 0.2 || p <= 6.0;
  }
  return all_in_interval;
}

TEST_CASE("PRG", "[misc]") {
  scl::PRG prg;

  REQUIRE(scl::PRG::BlockSize() == 16);
  REQUIRE(scl::PRG::SeedSize() == 16);

  SECTION("SanityCheck") {
    const unsigned count = 500;
    unsigned char buffer[count] = {0};
    prg.Next(buffer, count);
    bool all_in_interval = BufferLooksRandom(buffer, count);
    REQUIRE(all_in_interval);
  }

  SECTION("Stable") {
    unsigned char seed[scl::PRG::SeedSize()] = "1234567890abcde";
    scl::PRG prg0(seed);
    scl::PRG prg1(seed);

    REQUIRE(prg0.Counter() == prg1.Counter());
    auto counter_before = prg1.Counter();
    REQUIRE(BufferCmp(prg0.Seed(), seed, scl::PRG::SeedSize()));

    auto rand0 = prg0.Next(100);
    auto rand1 = prg1.Next(100);

    REQUIRE(rand0 == rand1);
    REQUIRE(counter_before != prg1.Counter());

    prg0.Reset();
    auto rand00 = prg0.Next(100);
    REQUIRE(rand00 == rand0);
  }

  SECTION("Fill") {
    std::vector<unsigned char> buffer(100);
    prg.Next(buffer, 50);
    bool last_is_zero = true;
    for (std::size_t i = 50; i < 100; i++) last_is_zero &= buffer[i] == 0;
    REQUIRE(last_is_zero);

    REQUIRE_THROWS_MATCHES(
        prg.Next(buffer, 101), std::invalid_argument,
        Catch::Matchers::Message("requested more randomness than dest.size()"));

    prg.Next(buffer);
    REQUIRE(BufferLooksRandom(buffer.data(), buffer.size()));
  }
}
