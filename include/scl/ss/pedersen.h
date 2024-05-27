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

#ifndef SCL_SS_PEDERSEN_H
#define SCL_SS_PEDERSEN_H

#include <utility>

#include "scl/math/array.h"
#include "scl/math/lagrange.h"
#include "scl/math/poly.h"
#include "scl/math/vector.h"
#include "scl/ss/shamir.h"
#include "scl/util/prg.h"

namespace scl::ss {

/**
 * @brief A secret share in the Pedersen VSS scheme.
 *
 * A PedersenShare for party \f$i\in\{0,\dots,n-1\}\f$ is a tuple
 * \f$(a,r,\mathbf{A})\f$ where \f$\mathbf{A}\f$ is a vector of Pedersen
 * commitments over a group \f$\mathbb{G}\f$, and \f$(a,r)\f$ are elements of
 * \f$\mathbb{Z}_{ord(\mathbb{G})}\f$ corresponding to the \f$i\f$'th opening.
 * That is, \f$aG+rH=\mathbf{A}[i]\f$, for suitable values \f$G,H\f$. The vector
 * of commitments only explicitly lists the \f$t+1\f$ first commitments (where
 * \f$t\f$ is the privacy threshold), but the rest can be computed easily, e.g.,
 * via \ref computeCommitmentForIndex
 */
template <typename GROUP>
struct PedersenShare {
  /**
   * @brief The group that commitments live in.
   */
  using Group = GROUP;

  /**
   * @brief The field that shares live in.
   */
  using Field = typename GROUP::ScalarField;

  /**
   * @brief The secret share and randomness.
   */
  math::Array<Field, 2> share;

  /**
   * @brief The commitments.
   */
  math::Vector<Group> commitments;

  /**
   * @brief Get the commitment randomness of this share.
   */
  Field getRand() const {
    return share[1];
  }

  /**
   * @brief Get the share part of this share.
   */
  Field getShare() const {
    return share[0];
  }
};

/**
 * @brief A secret sharing for the Pedersen VSS scheme.
 */
template <typename GROUP>
struct PedersenSharing {
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
  math::Vector<math::Array<Field, 2>> shares;

  /**
   * @brief The share commitments.
   */
  math::Vector<Group> commitments;

  /**
   * @brief Get the share of a particular party.
   * @param party_id the ID of the party.
   * @return \p party_id's Pedersen share.
   */
  PedersenShare<GROUP> getShare(std::size_t party_id) const {
    return {shares[party_id], commitments};
  }
};

/**
 * @brief Verifiably secret share a value using Pedersen VSS scheme.
 * @param secret the secret.
 * @param t the privacy threshold.
 * @param n the number of shares to create.
 * @param prg a PRG to use for creating randomness.
 * @param h a curve point used in the commitments.
 * @param randomness the random value to use for the secret.
 * @return a PedersenSharing of \p secret.
 */
template <typename T>
PedersenSharing<T> pedersenSecretShare(
    const typename PedersenSharing<T>::Field& secret,
    std::size_t t,
    std::size_t n,
    util::PRG& prg,
    const typename PedersenSharing<T>::Group& h,
    const typename PedersenSharing<T>::Field& randomness) {
  using F = typename PedersenSharing<T>::Field;
  using G = typename PedersenSharing<T>::Group;

  const math::Array<F, 2> s = {{secret, randomness}};
  const auto shares = shamirSecretShare(s, t, n, prg);

  std::vector<G> comm;
  comm.reserve(t + 1);
  const auto gen = G::generator();
  comm.emplace_back(secret * gen + randomness * h);
  for (std::size_t i = 0; i < t; ++i) {
    comm.emplace_back(shares[i][0] * gen + shares[i][1] * h);
  }

  return {shares, comm};
}

/**
 * @brief Verifiably secret share a value using Pedersen VSS scheme.
 * @param secret the secret.
 * @param t the privacy threshold.
 * @param n the number of shares to create.
 * @param prg a PRG to use for creating randomness.
 * @param h a curve point used in the commitments.
 * @return a PedersenSharing of \p secret.
 */
template <typename GROUP>
PedersenSharing<GROUP> pedersenSecretShare(
    const typename PedersenSharing<GROUP>::Field& secret,
    std::size_t t,
    std::size_t n,
    util::PRG& prg,
    const typename PedersenSharing<GROUP>::Group& h) {
  using F = typename PedersenSharing<GROUP>::Field;
  const auto rand = F::random(prg);
  return pedersenSecretShare<GROUP>(secret, t, n, prg, h, rand);
}

/**
 * @brief Compute the commitment for a particular index.
 * @param commitments the commitments of a Pedersen secret share.
 * @param share_index the index of the share.
 * @return the commitment of the share at \p share_index.
 */
template <typename GROUP>
GROUP computeCommitmentForIndex(const math::Vector<GROUP>& commitments,
                                std::size_t share_index) {
  if (share_index < commitments.size()) {
    return commitments[share_index];
  }

  using Field = typename PedersenShare<GROUP>::Field;
  using Group = typename PedersenShare<GROUP>::Group;

  const auto ns = math::Vector<Field>::range(commitments.size());
  const auto lb = math::computeLagrangeBasis(ns, share_index);
  return math::innerProd<Group>(lb.begin(), lb.end(), commitments.begin());
}

/**
 * @brief Verify a Pedersen secret share.
 * @param share the share to verify.
 * @param share_index the evaluation index of the share.
 * @param h the curve point used in the commitments.
 * @return true if the share is valid and false otherwise.
 */
template <typename GROUP>
bool pedersenVerify(const PedersenShare<GROUP> share,
                    std::size_t share_index,
                    const typename PedersenShare<GROUP>::Group& h) {
  using Group = typename PedersenShare<GROUP>::Group;
  return computeCommitmentForIndex(share.commitments, share_index) ==
         share.getShare() * Group::generator() + share.getRand() * h;
}

/**
 * @brief Verify a Pedersen secret share.
 * @param share the share and randomness to verify.
 * @param commitments the share commitments.
 * @param share_index the evaluation index of the share.
 * @param h the curve point used in the commitments.
 * @return true if the share is valid and false otherwise.
 */
template <typename T>
bool pedersenVerify(
    const math::Array<typename PedersenSharing<T>::Field, 2>& share,
    const math::Vector<typename PedersenSharing<T>::Group>& commitments,
    std::size_t share_index,
    const typename PedersenShare<T>::Group& h) {
  return pedersenVerify<T>({share, commitments}, share_index, h);
}

/**
 * @brief Apply a matrix to a vector of shares.
 * @param begin a beginning iterator to a list of shares.
 * @param end an end iterator to a list of shares.
 * @param matrix the matrix.
 * @return \p shares after multiplying with \p matrix.
 *
 * This function is useful if one wishes to randomize a vector of shares using
 * e.g., a Vandermonde matrix, as in DN07.
 */
template <typename T, typename IT>
std::vector<PedersenShare<T>> apply(
    const IT begin,
    const IT end,
    const math::Matrix<typename PedersenShare<T>::Field>& matrix) {
  // stupid case
  if (begin == end) {
    return {};
  }

  using Group = typename PedersenShare<T>::Group;

  const std::size_t n = matrix.rows();
  const std::size_t p = matrix.cols();
  const std::size_t m = begin->commitments.size();

  // multiply matrix from left
  std::vector<PedersenShare<T>> shares_out(n);
  for (auto& share_out : shares_out) {
    share_out.commitments = math::Vector<Group>(m);
  }

  std::size_t i;
  std::size_t k;
  std::size_t j;

  for (i = 0; i < n; i++) {
    auto b = begin;
    for (k = 0; k < p; k++) {
      shares_out[i].share += b->share * matrix(i, k);
      for (j = 0; j < m; j++) {
        shares_out[i].commitments[j] += matrix(i, k) * b->commitments[j];
      }
      b++;
    }
  }

  return shares_out;
}

/**
 * @brief Apply a matrix to a vector of shares.
 * @param shares the shares.
 * @param matrix the matrix.
 * @return \p shares after multiplying with \p matrix.
 */
template <typename T>
std::vector<PedersenShare<T>> apply(
    const std::vector<PedersenShare<T>>& shares,
    const math::Matrix<typename PedersenShare<T>::Field>& matrix) {
  return apply<T>(shares.begin(), shares.end(), matrix);
}

}  // namespace scl::ss

#endif  // SCL_SS_PEDERSEN_H
