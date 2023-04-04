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

#ifndef SCL_SS_FELDMAN_H
#define SCL_SS_FELDMAN_H

#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <type_traits>

#include "scl/math/lagrange.h"
#include "scl/math/vec.h"
#include "scl/ss/shamir.h"
#include "scl/util/prg.h"

namespace scl::ss {

/**
 * @brief A Feldman secret-sharing.
 */
template <typename G>
struct FeldmanSharing {
  /**
   * @brief The shares.
   */
  math::Vec<typename G::Order> shares;

  /**
   * @brief The commitments.
   */
  math::Vec<G> commitments;
};

/**
 * @brief Create a Feldman secret-sharing.
 * @param secret the secret to secret-share.
 * @param t the privacy threshold.
 * @param n the number of shares to create.
 * @param prg a PRG for creating randomness.
 * @return a Feldman secret-sharing.
 */
template <typename G>
FeldmanSharing<G> FeldmanShare(const typename G::Order& secret,
                               std::size_t t,
                               std::size_t n,
                               util::PRG& prg) {
  const auto shares = ShamirShare(secret, t, n, prg);

  std::vector<G> comm;
  comm.reserve(t + 1);
  const auto gen = G::Generator();
  for (std::size_t i = 0; i < t + 1; ++i) {
    comm.emplace_back(shares[i] * gen);
  }

  return {shares, math::Vec<G>{comm}};
}

/**
 * @brief A Feldman secret-share and the owner's index.
 */
template <typename G>
struct ShareAndIndex {
  /**
   * @brief The index.
   */
  std::size_t index;

  /**
   * @brief The share.
   */
  typename G::Order share;
};

/**
 * @brief Verify a share given a set of commitments.
 * @param share_and_index the secret-share and its index.
 * @param commits a set of commitments.
 * @return true if the provided share is valid for that index, and false
 * otherwise.
 *
 * This function checks if a provided share is consistent with a set of
 * commitments.
 */
template <typename G>
bool FeldmanVerify(const ShareAndIndex<G>& share_and_index,
                   const math::Vec<G>& commits) {
  const auto ns = math::Vec<typename G::Order>::Range(1, commits.Size() + 1);
  const auto lb = math::ComputeLagrangeBasis(ns, share_and_index.index);
  const auto v =
      math::UncheckedInnerProd<G>(lb.begin(), lb.end(), commits.begin());
  return v == G::Generator() * share_and_index.share;
}

}  // namespace scl::ss

#endif  // SCL_SS_FELDMAN_H
