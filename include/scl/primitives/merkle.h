/* SCL --- Secure Computation Library
 * Copyright (C) 2025 Anders Dalskov
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

#ifndef SCL_PRIMITIVES_MERKLE_H
#define SCL_PRIMITIVES_MERKLE_H

#include <vector>

#include "scl/bitmap.h"
#include "scl/primitives/hash.h"

namespace scl {

/**
 * @brief A Merkle tree proof.
 *
 * A MerkleProof is used to prove that a specific leaf recides in a Merkle tree,
 * represented by its root. A proof is a path through the tree, from the leaf in
 * question, to the root.
 */
template <typename DIGEST>
struct MerkleProof {
  /**
   * @brief The path from a particular leaf to the root.
   */
  std::vector<DIGEST> path;

  /**
   * @brief A vector describing whether at the left or right element for each
   * element in a path.
   */
  Bitmap direction;
};

/**
 * @brief Serializer for MerkleProof.
 */
template <typename DIGEST>
struct Serializer<MerkleProof<DIGEST>> {
  /**
   * @brief Determines the size in bytes of a merkle proof.
   */
  static std::size_t sizeOf(const MerkleProof<DIGEST>& proof) {
    return Serializer<std::vector<DIGEST>>::sizeOf(proof.path) +
           Serializer<Bitmap>::sizeOf(proof.direction);
  }

  /**
   * @brief Write a merkle proof to a buffer.
   */
  static std::size_t write(const MerkleProof<DIGEST>& proof,
                           unsigned char* buf) {
    buf += Serializer<std::vector<DIGEST>>::write(proof.path, buf);
    buf += Serializer<Bitmap>::write(proof.direction, buf);
    return sizeOf(proof);
  }

  /**
   * @brief Read a merkle proof from a buffer.
   */
  static std::size_t read(MerkleProof<DIGEST>& proof,
                          const unsigned char* buf) {
    buf += Serializer<std::vector<DIGEST>>::read(proof.path, buf);
    buf += Serializer<Bitmap>::read(proof.direction, buf);
    return sizeOf(proof);
  }
};

/**
 * @brief Merkle tree hash.
 *
 * MerkleTree can be used to construct a Merkle hash over a list of
 * values. MerkleTree is parameterized by two types: The leaf type, which must
 * be Serializable, and the hash function to use, which should be something
 * adhearing to the IUFHAsh interface.
 *
 * @code
 * #include <scl/primitives/merkle.h>
 *
 * std::vector<LEAF> leafs = ...
 * LEAF specific_leaf = ...
 *
 * leafs[42] = specific_leaf;
 *
 * auto merkle_hash = scl::MerkleTree<LEAF>::hash(leafs);
 *
 * // create proof that specific_leaf recides at index 42.
 * auto proof = scl::MerkleTree<LEAF>::prove(leafs, 42);
 *
 * // verify the proof
 * assert(scl::MerkleTree<LEAF>::verify(specific_leaf, merkle_hash, proof));
 * @endcode
 */
template <typename LEAF, typename HASH = Hash<256>>
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
   */
  static DigestType hash(const std::vector<LEAF>& data);

  /**
   * @brief Create a proof that a particular index is part of a Merkle tree.
   */
  static Proof prove(const std::vector<LEAF>& data, std::size_t index);

  /**
   * @brief Verify a Merkle tree proof.
   */
  static bool verify(const LEAF& leaf,
                     const DigestType& root,
                     const Proof& proof);

 private:
  static std::vector<DigestType> hashLeafs(const std::vector<LEAF>& data);
};

template <typename LEAF, typename HASH>
auto MerkleTree<LEAF, HASH>::hashLeafs(const std::vector<LEAF>& data)
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
  }

  return digests;
}

template <typename LEAF, typename HASH>
auto MerkleTree<LEAF, HASH>::hash(const std::vector<LEAF>& data) -> DigestType {
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

template <typename LEAF, typename HASH>
auto MerkleTree<LEAF, HASH>::prove(const std::vector<LEAF>& data,
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

template <typename LEAF, typename HASH>
bool MerkleTree<LEAF, HASH>::verify(const LEAF& leaf,
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

}  // namespace scl

#endif  // SCL_PRIMITIVES_MERKLE_H
