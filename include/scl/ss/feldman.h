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

#ifndef SCL_SS_FELDMAN_H
#define SCL_SS_FELDMAN_H

#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <type_traits>

#include "scl/math/lagrange.h"
#include "scl/math/vector.h"
#include "scl/ss/shamir.h"
#include "scl/util/prg.h"

namespace scl::ss {

/**
 * @brief A verifiable secret share for Feldman VSSS.
 */
template <typename GROUP>
struct FeldmanShare {
  /**
   * @brief The group that commitments live in.
   */
  using Group = GROUP;

  /**
   * @brief The field that shares live in.
   */
  using Field = typename GROUP::ScalarField;

  /**
   * @brief The share.
   */
  Field share;

  /**
   * @brief The commitments.
   */
  math::Vector<Group> commitments;
};

/**
 * @brief A verifiable secret-sharing suitable for Feldman VSSS.
 *
 * This struct captures a set of secret shares produced by the Feldman
 * verifiable secret sharing schemes. In this scheme, a secret is shared into
 * \f$n\f$ shares and \f$t+1\f$ commitments. The share held by a party is one of
 * the \f$n\f$ shares, and all \f$t+1\f$ commitments.
 */
template <typename GROUP>
struct FeldmanSharing {
  /**
   * @brief The group that commitments live in.
   */
  using Group = GROUP;

  /**
   * @brief The field that shares live in.
   */
  using Field = typename GROUP::ScalarField;

  /**
   * @brief The shares.
   */
  math::Vector<typename GROUP::ScalarField> shares;

  /**
   * @brief The commitments.
   */
  math::Vector<GROUP> commitments;

  /**
   * @brief Get a particular party's share.
   * @param party_id the ID of the party.
   * @return \p party_id's share.
   */
  FeldmanShare<GROUP> getShare(std::size_t party_id) const {
    return {shares[party_id], commitments};
  }
};

/**
 * @brief Create a Feldman secret-sharing.
 * @param secret the secret to secret-share.
 * @param t the privacy threshold.
 * @param n the number of shares to create.
 * @param prg a PRG for creating randomness.
 * @return a Feldman secret-sharing.
 */
template <typename GROUP>
FeldmanSharing<GROUP> feldmanSecretShare(
    const typename FeldmanSharing<GROUP>::Field& secret,
    std::size_t t,
    std::size_t n,
    util::PRG& prg) {
  const auto shares = shamirSecretShare(secret, t, n, prg);

  std::vector<GROUP> comm;
  comm.reserve(t + 1);
  const auto gen = GROUP::generator();
  comm.emplace_back(secret * gen);
  for (std::size_t i = 0; i < t; ++i) {
    comm.emplace_back(shares[i] * gen);
  }

  return {shares, math::Vector<GROUP>{comm}};
}

/**
 * @brief Verify a share given a set of commitments.
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
  const auto ns = math::Vector<F>::range(share.commitments.size());
  const auto lb = math::computeLagrangeBasis(ns, share_index);
  const auto v =
      math::innerProd<GROUP>(lb.begin(), lb.end(), share.commitments.begin());
  return v == GROUP::generator() * share.share;
}

/**
 * @brief Verify a share given a set of commitments.
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
    const math::Vector<typename FeldmanShare<GROUP>::Group>& commitments,
    std::size_t share_index) {
  return feldmanVerify<GROUP>({share, commitments}, share_index);
}

}  // namespace scl::ss

#endif  // SCL_SS_FELDMAN_H
