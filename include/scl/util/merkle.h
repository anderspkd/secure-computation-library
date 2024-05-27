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

#ifndef SCL_UTIL_MERKLE_H
#define SCL_UTIL_MERKLE_H

#include <vector>

#include "scl/util/bitmap.h"
#include "scl/util/digest.h"
#include "scl/util/merkle_proof.h"

namespace scl::util {

/**
 * @brief Merkle hash tree.
 * @tparam H a hash function.
 * @tparam T the leaf data type.
 */
template <typename HASH, typename LEAF>
struct MerkleTree {
  /**
   * @brief The digest type nodes.
   */
  using DigestType = typename HASH::DigestType;

  /**
   * @brief The proof type.
   */
  using Proof = MerkleProof<DigestType>;

  /**
   * @brief Compute a Merkle tree hash.
   * @param data the date to hash.
   * @return the root hash.
   */
  static DigestType hash(const std::vector<LEAF>& data);

  /**
   * @brief Create a proof that a particular index is part of a Merkle tree.
   */
  static Proof prove(const std::vector<LEAF>& data, std::size_t index);

  /**
   * @brief Verify a Merkle tree proof.
   * @param leaf the statement.
   * @param root the tree root.
   * @param proof the proof
   * @return true if the
   */
  static bool verify(const LEAF& leaf,
                     const DigestType& root,
                     const Proof& proof);

 private:
  static std::vector<DigestType> hashLeafs(const std::vector<LEAF>& data);
};

template <typename HASH, typename LEAF>
auto MerkleTree<HASH, LEAF>::hashLeafs(const std::vector<LEAF>& data)
    -> std::vector<DigestType> {
  std::vector<DigestType> digests;
  auto sz = data.size();
  digests.reserve(sz);

  for (const auto& d : data) {
    HASH hash;
    digests.emplace_back(hash.update(d).finalize());
  }

  // duplicate the last hash in case there's an odd number of leafs.
  if (data.size() % 2 == 1) {
    digests.emplace_back(digests.back());
    sz++;
  }

  return digests;
}  // LCOV_EXCL_LINE

template <typename HASH, typename LEAF>
auto MerkleTree<HASH, LEAF>::hash(const std::vector<LEAF>& data) -> DigestType {
  std::vector<DigestType> digests = hashLeafs(data);

  auto sz = digests.size();

  while (sz > 1) {
    std::size_t j = 0;
    for (std::size_t i = 0; i < sz; i += 2) {
      const auto left = digests[i];
      const auto right = digests[i + 1];
      HASH hash;
      digests[j] = hash.update(left).update(right).finalize();
      j++;
    }

    sz /= 2;

    // Duplicate the last node if there's an odd number of leafs.
    if (sz > 1 && sz % 2 == 1) {
      digests[j] = digests[j - 1];
      sz++;
    }
  }

  return digests[0];
}

template <typename HASH, typename LEAF>
auto MerkleTree<HASH, LEAF>::prove(const std::vector<LEAF>& data,
                                   std::size_t index) -> Proof {
  std::vector<DigestType> digests = hashLeafs(data);
  std::vector<DigestType> path;
  std::vector<bool> direction;

  auto sz = digests.size();

  while (sz > 1) {
    std::size_t j = 0;
    for (std::size_t i = 0; i < sz; i += 2) {
      const auto left = digests[i];
      const auto right = digests[i + 1];

      HASH hash;
      digests[j] = hash.update(left).update(right).finalize();

      if (i == index) {
        path.emplace_back(right);
        direction.emplace_back(false);
        index = j;
      } else if (i + 1 == index) {
        path.emplace_back(left);
        direction.emplace_back(true);
        index = j;
      }

      j++;
    }

    sz /= 2;

    if (sz > 1 && sz % 2 == 1) {
      digests[j] = digests[j - 1];
      sz++;
    }
  }

  return {path, Bitmap::fromStdVecBool(direction)};
}

template <typename HASH, typename LEAF>
bool MerkleTree<HASH, LEAF>::verify(const LEAF& leaf,
                                    const DigestType& root,
                                    const Proof& proof) {
  const auto [h, d] = proof;

  auto digest = HASH{}.update(leaf).finalize();
  for (std::size_t i = 0; i < h.size(); ++i) {
    HASH hash;
    if (d.at(i)) {
      digest = hash.update(h[i]).update(digest).finalize();
    } else {
      digest = hash.update(digest).update(h[i]).finalize();
    }
  }

  return root == digest;
}

}  // namespace scl::util

#endif  // SCL_UTIL_MERKLE_H
