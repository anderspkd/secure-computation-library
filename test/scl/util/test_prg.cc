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

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include "scl/util/prg.h"

using namespace scl;

namespace {

bool looksUniform(const unsigned char* p, std::size_t len) {
  std::vector<unsigned int> buckets(256, 0);
  for (std::size_t i = 0; i < len; ++i) {
    buckets[p[i]]++;
  }

  bool uniform = true;
  for (unsigned int& bv : buckets) {
    auto v = 100 * ((double)bv / (double)len);
    uniform &= (v >= 0.35) && (v <= 0.65);
  }

  return uniform;
}

}  // namespace

TEST_CASE("PRG construction", "[misc]") {
  REQUIRE(util::PRG::seedSize() == 16);

  auto zprg = util::PRG::create();

  // Number arrived at somewhat by trial-and-error
  const auto n = 300000;
  unsigned char buffer[n] = {0};
  REQUIRE_FALSE(looksUniform(buffer, n));

  zprg.next(buffer, n);

  REQUIRE(looksUniform(buffer, n));
}

TEST_CASE("PRG predictable", "[misc]") {
  unsigned char seed[] = "1234567890abcde";
  auto prg0 = util::PRG::create(seed, 15);
  auto prg1 = util::PRG::create(seed, 15);

  REQUIRE(prg0.Seed() == prg1.Seed());

  auto bytes0 = prg0.next(100);
  auto bytes1 = prg1.next(100);

  REQUIRE(bytes0 == bytes1);

  prg0.reset();
  auto bytes00 = prg0.next(100);
  REQUIRE(bytes00 == bytes0);
}

TEST_CASE("PRG generate random bytes", "[misc]") {
  auto prg = util::PRG::create();

  std::vector<unsigned char> buffer(100, 0);
  prg.next(buffer, 50);

  bool zero = true;
  for (std::size_t i = 50; i < 100; ++i) {
    zero &= buffer[i] == 0;
  }
  REQUIRE(zero);

  // very weak check
  bool non_zero = false;
  for (std::size_t i = 0; i < 50; ++i) {
    non_zero |= buffer[i] != 0;
  }
  REQUIRE(non_zero);

  std::vector<unsigned char> buf = {'c', 'a', 't'};

  prg.next(buf, 0);
  REQUIRE(buf == std::vector<unsigned char>{'c', 'a', 't'});
}

TEST_CASE("PRG invalid calls", "[misc]") {
  auto prg = util::PRG::create();
  std::vector<unsigned char> buf(10);

  REQUIRE_THROWS_MATCHES(prg.next(buf, 11),
                         std::invalid_argument,
                         Catch::Matchers::Message("n exceeds buffer.size()"));
}

TEST_CASE("PRG truncate seed on create", "[misc]") {
  // Seeds are truncated if they exceed PRG::SeedSize() length.

  auto prg0 = util::PRG::create("0123456789abcdef_bar");
  auto prg1 = util::PRG::create("0123456789abcdef_foo");

  auto bytes0 = prg0.next(100);
  auto bytes1 = prg1.next(100);

  REQUIRE(bytes0 == bytes1);
}
