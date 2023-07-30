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

#ifndef SCL_UTIL_MERKLE_H
#define SCL_UTIL_MERKLE_H

#include <vector>

#include "scl/util/digest.h"

namespace scl::util {

/**
 * @brief Merkle hash tree.
 * @tparam H a hash function.
 * @tparam T the leaf data type.
 */
template <typename H, typename T>
struct MerkleTree {
  /**
   * @brief The digest type nodes.
   */
  using DigestType = typename H::DigestType;

  /**
   * @brief Compute a Merkle tree hash.
   * @param data the date to hash.
   * @return the root hash.
   */
  static DigestType Hash(const std::vector<T>& data);

  /**
   * @brief A Merkle tree proof.
   */
  struct Proof {
    /**
     * @brief The path from a particular leaf to the root.
     */
    std::vector<DigestType> path;

    /**
     * @brief A vector describing whether at the left or right element for each
     * element in a path.
     */
    std::vector<bool> direction;
  };

  /**
   * @brief Create a proof that a particular index is part of a Merkle tree.
   */
  static Proof Prove(const std::vector<T>& data, std::size_t index);

  /**
   * @brief Verify a Merkle tree proof.
   * @param value the statement.
   * @param root the tree root.
   * @param proof the proof
   * @return true if the
   */
  static bool Verify(const T& value,
                     const DigestType& root,
                     const Proof& proof);

 private:
  static std::vector<DigestType> HashLeafs(const std::vector<T>& data);
};

template <typename H, typename T>
auto MerkleTree<H, T>::HashLeafs(const std::vector<T>& data)
    -> std::vector<DigestType> {
  std::vector<DigestType> digests;
  auto sz = data.size();
  digests.reserve(sz);

  for (const auto& d : data) {
    H hash;
    digests.emplace_back(hash.Update(d).Finalize());
  }

  // duplicate the last hash in case there's an odd number of leafs.
  if (data.size() % 2 == 1) {
    digests.emplace_back(digests.back());
    sz++;
  }

  return digests;
}  // LCOV_EXCL_LINE

template <typename H, typename T>
auto MerkleTree<H, T>::Hash(const std::vector<T>& data) -> DigestType {
  std::vector<DigestType> digests = HashLeafs(data);

  auto sz = digests.size();

  while (sz > 1) {
    std::size_t j = 0;
    for (std::size_t i = 0; i < sz; i += 2) {
      const auto left = digests[i];
      const auto right = digests[i + 1];
      H hash;
      digests[j] = hash.Update(left).Update(right).Finalize();
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

template <typename H, typename T>
auto MerkleTree<H, T>::Prove(const std::vector<T>& data, std::size_t index)
    -> Proof {
  std::vector<DigestType> digests = HashLeafs(data);
  std::vector<DigestType> path;
  std::vector<bool> direction;

  auto sz = digests.size();

  while (sz > 1) {
    std::size_t j = 0;
    for (std::size_t i = 0; i < sz; i += 2) {
      const auto left = digests[i];
      const auto right = digests[i + 1];

      H hash;
      digests[j] = hash.Update(left).Update(right).Finalize();

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

  return {path, direction};
}

template <typename H, typename T>
bool MerkleTree<H, T>::Verify(const T& value,
                              const DigestType& root,
                              const Proof& proof) {
  const auto [h, d] = proof;

  auto digest = H{}.Update(value).Finalize();
  for (std::size_t i = 0; i < h.size(); ++i) {
    H hash;
    if (d[i]) {
      digest = hash.Update(h[i]).Update(digest).Finalize();
    } else {
      digest = hash.Update(digest).Update(h[i]).Finalize();
    }
  }

  return root == digest;
}

}  // namespace scl::util

#endif  // SCL_UTIL_MERKLE_H
