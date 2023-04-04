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

#include "scl/util/digest.h"
#include "scl/util/hash.h"

using namespace scl;

TEST_CASE("Sha3 empty hash", "[misc]") {
  static const util::Digest<256>::Type SHA3_256_empty = {
      0xa7, 0xff, 0xc6, 0xf8, 0xbf, 0x1e, 0xd7, 0x66, 0x51, 0xc1, 0x47,
      0x56, 0xa0, 0x61, 0xd6, 0x62, 0xf5, 0x80, 0xff, 0x4d, 0xe4, 0x3b,
      0x49, 0xfa, 0x82, 0xd8, 0x0a, 0x4b, 0x80, 0xf8, 0x43, 0x4a};

  util::Hash<256> hash;
  auto digest = hash.Finalize();
  REQUIRE(digest == SHA3_256_empty);
}

TEST_CASE("Sha3 abc hash", "[misc]") {
  static const util::Digest<256>::Type SHA3_256_abc = {
      0x3a, 0x98, 0x5d, 0xa7, 0x4f, 0xe2, 0x25, 0xb2, 0x04, 0x5c, 0x17,
      0x2d, 0x6b, 0xd3, 0x90, 0xbd, 0x85, 0x5f, 0x08, 0x6e, 0x3e, 0x9d,
      0x52, 0x5b, 0x46, 0xbf, 0xe2, 0x45, 0x11, 0x43, 0x15, 0x32};

  util::Hash<256> hash;
  unsigned char abc[] = "abc";
  auto digest = hash.Update(abc, 3).Finalize();
  REQUIRE(digest == SHA3_256_abc);
}

TEST_CASE("Sha3-256 reference", "[misc]") {
  static const util::Digest<256>::Type SHA3_256_0xa3_200_times = {
      0x79, 0xf3, 0x8a, 0xde, 0xc5, 0xc2, 0x03, 0x07, 0xa9, 0x8e, 0xf7,
      0x6e, 0x83, 0x24, 0xaf, 0xbf, 0xd4, 0x6c, 0xfd, 0x81, 0xb2, 0x2e,
      0x39, 0x73, 0xc6, 0x5f, 0xa1, 0xbd, 0x9d, 0xe3, 0x17, 0x87};

  unsigned char byte = 0xA3;
  unsigned char buf[200];
  for (std::size_t i = 0; i < 200; ++i) {
    buf[i] = byte;
  }

  util::Hash<256> hash0;
  auto digest = hash0.Update(buf, 200).Finalize();
  REQUIRE(digest == SHA3_256_0xa3_200_times);

  util::Hash<256> hash1;
  for (std::size_t i = 0; i < 200; ++i) {
    hash1.Update(&byte, 1);
  }
  REQUIRE(hash1.Finalize() == SHA3_256_0xa3_200_times);
}

TEST_CASE("Sha3-384 reference", "[misc]") {
  static const util::Digest<384>::Type SHA3_384_0xa3_200_times = {
      0x18, 0x81, 0xde, 0x2c, 0xa7, 0xe4, 0x1e, 0xf9, 0x5d, 0xc4, 0x73, 0x2b,
      0x8f, 0x5f, 0x00, 0x2b, 0x18, 0x9c, 0xc1, 0xe4, 0x2b, 0x74, 0x16, 0x8e,
      0xd1, 0x73, 0x26, 0x49, 0xce, 0x1d, 0xbc, 0xdd, 0x76, 0x19, 0x7a, 0x31,
      0xfd, 0x55, 0xee, 0x98, 0x9f, 0x2d, 0x70, 0x50, 0xdd, 0x47, 0x3e, 0x8f};

  unsigned char byte = 0xA3;
  unsigned char buf[200];
  for (std::size_t i = 0; i < 200; ++i) {
    buf[i] = byte;
  }

  util::Hash<384> hash0;
  auto digest = hash0.Update(buf, 200).Finalize();
  REQUIRE(digest.size() == 48);
  REQUIRE(digest == SHA3_384_0xa3_200_times);

  util::Hash<384> hash1;
  for (std::size_t i = 0; i < 200; ++i) {
    hash1.Update(&byte, 1);
  }
  REQUIRE(hash1.Finalize() == SHA3_384_0xa3_200_times);
}

TEST_CASE("Sha3-512 reference", "[misc]") {
  static const std::array<unsigned char, 512 / 8> SHA3_512_0xa3_200_times = {
      0xe7, 0x6d, 0xfa, 0xd2, 0x20, 0x84, 0xa8, 0xb1, 0x46, 0x7f, 0xcf,
      0x2f, 0xfa, 0x58, 0x36, 0x1b, 0xec, 0x76, 0x28, 0xed, 0xf5, 0xf3,
      0xfd, 0xc0, 0xe4, 0x80, 0x5d, 0xc4, 0x8c, 0xae, 0xec, 0xa8, 0x1b,
      0x7c, 0x13, 0xc3, 0x0a, 0xdf, 0x52, 0xa3, 0x65, 0x95, 0x84, 0x73,
      0x9a, 0x2d, 0xf4, 0x6b, 0xe5, 0x89, 0xc5, 0x1c, 0xa1, 0xa4, 0xa8,
      0x41, 0x6d, 0xf6, 0x54, 0x5a, 0x1c, 0xe8, 0xba, 0x00};

  unsigned char byte = 0xA3;
  unsigned char buf[200];
  for (std::size_t i = 0; i < 200; ++i) {
    buf[i] = byte;
  }

  util::Hash<512> hash0;
  auto digest = hash0.Update(buf, 200).Finalize();
  REQUIRE(digest.size() == 64);
  REQUIRE(digest == SHA3_512_0xa3_200_times);

  util::Hash<512> hash1;
  for (std::size_t i = 0; i < 200; ++i) {
    hash1.Update(&byte, 1);
  }
  REQUIRE(hash1.Finalize() == SHA3_512_0xa3_200_times);
}

TEST_CASE("Sha3 hash vector", "[misc]") {
  unsigned char ref_buf[] = "hello, world";
  util::Hash<256> hash_ref;
  auto ref = hash_ref.Update(ref_buf, 12).Finalize();

  util::Hash<256> hash1;
  std::vector<unsigned char> v =
      {'h', 'e', 'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd'};
  auto from_vec = hash1.Update(v).Finalize();
  REQUIRE(ref == from_vec);
}

TEST_CASE("Sha3 hash array", "[misc]") {
  unsigned char abc[] = "abc";
  std::array<unsigned char, 3> abc_arr = {'a', 'b', 'c'};
  auto ref = util::Hash<256>{}.Update(abc, 3).Finalize();
  auto act = util::Hash<256>{}.Update(abc_arr).Finalize();
  REQUIRE(ref == act);
}
