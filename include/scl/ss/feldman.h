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

#pragma once

#include <cstddef>

#include "scl/math/lagrange.h"
#include "scl/math/vector.h"
#include "scl/primitives/prg.h"
#include "scl/ss/shamir.h"

namespace scl {

/**
 * @brief A verifiable secret share for Feldman VSSS.
 * @ingroup ss
 */
template <typename GROUP>
struct FeldmanShare {
  /**
   * @brief The commitment type.
   */
  using CommitType = GROUP;

  /**
   * @brief The share type.
   */
  using ShareType = typename GROUP::ScalarField;

  /**
   * @brief The secret type.
   */
  using SecretType = typename GROUP::ScalarField;

  /**
   * @brief The share.
   */
  ShareType share;

  /**
   * @brief The commitments.
   */
  Vector<CommitType> commitments;
};

/**
 * @brief A verifiable secret-sharing suitable for Feldman VSSS.
 * @ingroup ss
 */
template <typename GROUP>
struct FeldmanSharing {
  /**
   * @brief The shares.
   */
  Vector<typename FeldmanShare<GROUP>::ShareType> shares;

  /**
   * @brief The commitments.
   */
  Vector<typename FeldmanShare<GROUP>::CommitType> commitments;
};

/**
 * @brief Create a Feldman secret-sharing.
 * @ingroup ss
 * @param secret the secret to secret-share.
 * @param t the privacy threshold.
 * @param n the number of shares to create.
 * @param prg a PRG for creating randomness.
 * @return a Feldman secret-sharing.
 */
template <typename GROUP>
FeldmanSharing<GROUP> createFeldmanVerifiableSharing(
    const typename FeldmanShare<GROUP>::SecretType& secret,
    std::size_t t,
    std::size_t n,
    PRG& prg) {
  const auto shares = createShamirSharing(secret, t, n, prg);

  std::vector<GROUP> comm;
  comm.reserve(t + 1);
  const auto gen = GROUP::generator();
  comm.emplace_back(secret * gen);
  for (std::size_t i = 0; i < t; ++i) {
    comm.emplace_back(shares[i] * gen);
  }

  return {shares, Vector<GROUP>{comm}};
}

/**
 * @brief Verify a share given a set of commitments.
 * @ingroup ss
 * @param share the share to verify.
 * @param share_index the index (e.g., party ID) of the share.
 * @return true if the provided share is valid for that index, and false
 * otherwise.
 *
 * This function checks if a provided share is consistent with a set of
 * commitments.
 */
template <typename GROUP>
bool feldmanVerify(const FeldmanShare<GROUP>& share, std::size_t share_index) {
  using F = typename GROUP::ScalarField;
  const auto ns = Vector<F>::range(share.commitments.size());
  const auto lb = computeLagrangeBasis(ns, share_index);
  const auto v =
      innerProd<GROUP>(lb.begin(), lb.end(), share.commitments.begin());
  return v == GROUP::generator() * share.share;
}

/**
 * @brief Verify a share given a set of commitments.
 * @ingroup ss
 * @param share the share to verify.
 * @param commitments the commitments to verify against.
 * @param share_index the index (e.g., party ID) of the share.
 * @return true if the provided share is valid for that index, and false
 * otherwise.
 *
 * This function checks if a provided share is consistent with a set of
 * commitments.
 */
template <typename GROUP>
bool feldmanVerify(
    const typename FeldmanShare<GROUP>::Field& share,
    const Vector<typename FeldmanShare<GROUP>::Group>& commitments,
    std::size_t share_index) {
  return feldmanVerify<GROUP>({share, commitments}, share_index);
}

}  // namespace scl
