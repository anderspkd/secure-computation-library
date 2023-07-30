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

#include "scl/util/hash.h"
#include "scl/util/merkle.h"

using namespace scl;

using Mrkl = util::MerkleTree<util::Hash<256>, std::string_view>;

namespace {

util::Hash<256>::DigestType Hash(std::string_view thing) {
  return util::Hash<256>{}.Update(thing).Finalize();
}

util::Hash<256>::DigestType Hash(util::Hash<256>::DigestType a,
                                 util::Hash<256>::DigestType b) {
  return util::Hash<256>{}.Update(a).Update(b).Finalize();
}

}  // namespace

TEST_CASE("Merkle hash", "[misc]") {
  auto h_abcd = Hash(Hash(Hash("a"), Hash("b")), Hash(Hash("c"), Hash("d")));
  auto m_abcd = Mrkl::Hash({"a", "b", "c", "d"});
  REQUIRE(h_abcd == m_abcd);

  auto h_xyvu = Hash(Hash(Hash("x"), Hash("y")), Hash(Hash("v"), Hash("u")));
  auto h_abcdxyvu = Hash(h_abcd, h_xyvu);

  auto m_abcdxyvu = Mrkl::Hash({"a", "b", "c", "d", "x", "y", "v", "u"});
  REQUIRE(h_abcdxyvu == m_abcdxyvu);
}

TEST_CASE("Merkle hash odd size input", "[misc]") {
  util::Hash<256>::DigestType z_digest;
  z_digest.fill(0);
  auto h_abc = Hash(Hash(Hash("a"), Hash("b")), Hash(Hash("c"), Hash("c")));
  auto m_abc = Mrkl::Hash({"a", "b", "c"});

  REQUIRE(h_abc == m_abc);
}

TEST_CASE("Merkle prove", "[misc]") {
  std::vector<std::string_view> data = {"a", "b", "c", "d", "e"};
  auto root = Mrkl::Hash(data);

  auto h_ab = Hash(Hash("a"), Hash("b"));
  auto h_cd = Hash(Hash("c"), Hash("d"));
  auto h_ee = Hash(Hash("e"), Hash("e"));
  auto h_abcd = Hash(h_ab, h_cd);
  auto h_eeee = Hash(h_ee, h_ee);

  REQUIRE(root == Hash(h_abcd, h_eeee));

  auto proof = Mrkl::Prove(data, 3);

  // path = [H_c, H_ab, H_eeee]
  // direction = [left, left, right] (true, true, false)

  REQUIRE(proof.direction.size() == 3);
  REQUIRE(proof.path.size() == 3);

  REQUIRE(proof.direction == std::vector<bool>{true, true, false});

  REQUIRE(proof.path[0] == Hash("c"));
  REQUIRE(proof.path[1] == h_ab);
  REQUIRE(proof.path[2] == h_eeee);

  REQUIRE(Mrkl::Verify("d", root, proof));
}
