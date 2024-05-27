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

#include <catch2/catch_test_macros.hpp>
#include <iostream>

#include "scl/math/curves/secp256k1.h"
#include "scl/math/ec.h"
#include "scl/util/digest.h"
#include "scl/util/sha256.h"

using namespace scl;

TEST_CASE("Sha256 empty hash", "[misc]") {
  const static std::array<unsigned char, 32> SHA256_empty = {
      0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14, 0x9a, 0xfb, 0xf4,
      0xc8, 0x99, 0x6f, 0xb9, 0x24, 0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b,
      0x93, 0x4c, 0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55};

  util::Sha256 hash;
  auto digest = hash.finalize();
  REQUIRE(digest.size() == 32);
  REQUIRE(digest == SHA256_empty);
}

TEST_CASE("Sha256 abc hash", "[misc]") {
  const static std::array<unsigned char, 32> SHA256_abc = {
      0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 0x41, 0x41, 0x40,
      0xde, 0x5d, 0xae, 0x22, 0x23, 0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17,
      0x7a, 0x9c, 0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad};

  util::Sha256 hash;
  hash.update({'a', 'b', 'c'});
  auto digest = hash.finalize();
  REQUIRE(digest.size() == 32);
  REQUIRE(digest == SHA256_abc);

  util::Sha256 hash_;
  hash_.update({'a', 'b'});
  hash_.update({'c'});
  auto digest_ = hash_.finalize();
  REQUIRE(digest_.size() == 32);
  REQUIRE(digest_ == SHA256_abc);
}

TEST_CASE("Sha256 hash almost complete chunk", "[misc]") {
  const static std::array<unsigned char, 32> digest = {
      0x65, 0xa1, 0x6c, 0xb7, 0x86, 0x13, 0x35, 0xd5, 0xac, 0xe3, 0xc6,
      0x07, 0x18, 0xb5, 0x05, 0x2e, 0x44, 0x66, 0x07, 0x26, 0xda, 0x4c,
      0xd1, 0x3b, 0xb7, 0x45, 0x38, 0x1b, 0x23, 0x5a, 0x17, 0x85};

  const unsigned char data[57] = {0};
  util::Sha256 hash;
  hash.update(data, 57);
  REQUIRE(hash.finalize() == digest);
}

TEST_CASE("Sha256 bouncycastle reference", "[misc]") {
  // Reference test showing that serialization + hashing is the same as
  // bouncycastle in Java.

  using Curve = math::EC<math::ec::Secp256k1>;
  auto pk = Curve::generator() * math::Number::fromString("a");

  const auto n = Curve::byteSize(false);
  unsigned char buf[n] = {0};
  pk.write(buf, false);

  util::Sha256 hash;
  hash.update(buf, n);

  auto d = hash.finalize();

  std::array<unsigned char, 32> target = {
      0xde, 0xc1, 0x6a, 0xc2, 0x78, 0x99, 0xeb, 0xdf, 0x76, 0x0e, 0xaf,
      0x0a, 0x9f, 0x30, 0x95, 0xd1, 0x6a, 0x55, 0xea, 0x59, 0xef, 0x2a,
      0xe1, 0x8e, 0x9d, 0x22, 0x33, 0xd6, 0xbe, 0x82, 0x58, 0x38};

  REQUIRE(d == target);
}
