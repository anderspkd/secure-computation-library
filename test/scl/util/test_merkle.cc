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

#include "scl/serialization/serializer.h"
#include "scl/util/bitmap.h"
#include "scl/util/hash.h"
#include "scl/util/merkle.h"

using namespace scl;

using Mrkl = util::MerkleTree<util::Hash<256>, std::string_view>;

namespace {

util::Hash<256>::DigestType hash(std::string_view data) {
  return util::Hash<256>{}.update(data).finalize();
}

util::Hash<256>::DigestType hash(util::Hash<256>::DigestType left,
                                 util::Hash<256>::DigestType right) {
  return util::Hash<256>{}.update(left).update(right).finalize();
}

}  // namespace

TEST_CASE("Merkle hash", "[misc]") {
  // clang-format off
  auto h_abcd = hash(
    hash(hash("a"), hash("b")),
    hash(hash("c"), hash("d")));
  // clang-format on

  auto m_abcd = Mrkl::hash({"a", "b", "c", "d"});
  REQUIRE(h_abcd == m_abcd);

  // clang-format off
  auto h_xyvu = hash(
    hash(hash("x"), hash("y")),
    hash(hash("v"), hash("u")));
  // clang-format on

  auto h_abcdxyvu = hash(h_abcd, h_xyvu);

  auto m_abcdxyvu = Mrkl::hash({"a", "b", "c", "d", "x", "y", "v", "u"});
  REQUIRE(h_abcdxyvu == m_abcdxyvu);
}

TEST_CASE("Merkle hash odd size input", "[misc]") {
  util::Hash<256>::DigestType z_digest;
  z_digest.fill(0);
  // clang-format off
  auto h_abc = hash(
    hash(hash("a"), hash("b")),
    hash(hash("c"), hash("c")));
  // clang-format on

  auto m_abc = Mrkl::hash({"a", "b", "c"});

  REQUIRE(h_abc == m_abc);
}

TEST_CASE("Merkle prove", "[misc]") {
  std::vector<std::string_view> data = {"a", "b", "c", "d", "e"};
  auto root = Mrkl::hash(data);

  auto h_ab = hash(hash("a"), hash("b"));
  auto h_cd = hash(hash("c"), hash("d"));
  auto h_ee = hash(hash("e"), hash("e"));
  auto h_abcd = hash(h_ab, h_cd);
  auto h_eeee = hash(h_ee, h_ee);

  REQUIRE(root == hash(h_abcd, h_eeee));

  auto proof = Mrkl::prove(data, 3);

  // path = [H_c, H_ab, H_eeee]
  // direction = [left, left, right] (true, true, false)

  REQUIRE(proof.path.size() == 3);

  REQUIRE(proof.direction ==
          util::Bitmap::fromStdVecBool(std::vector<bool>{true, true, false}));

  REQUIRE(proof.path[0] == hash("c"));
  REQUIRE(proof.path[1] == h_ab);
  REQUIRE(proof.path[2] == h_eeee);

  REQUIRE(Mrkl::verify("d", root, proof));

  using Sr = seri::Serializer<Mrkl::Proof>;

  // two vectors. One with three digests, and one with 3 bits that fit into one
  // byte.
  REQUIRE(Sr::sizeOf(proof) == 2 * sizeof(seri::StlVecSizeType) + 3L * 32 + 1);

  unsigned char buf[2 * sizeof(seri::StlVecSizeType) + 3L * 32 + 1];
  Sr::write(proof, buf);

  Mrkl::Proof p;
  Sr::read(p, buf);

  REQUIRE(p.direction == proof.direction);
  REQUIRE(p.path.size() == proof.path.size());
  for (std::size_t i = 0; i < proof.path.size(); ++i) {
    REQUIRE(p.path[i] == proof.path[i]);
  }
}
