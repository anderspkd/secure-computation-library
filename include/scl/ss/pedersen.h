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

#ifndef SCL_SS_PEDERSEN_H
#define SCL_SS_PEDERSEN_H

#include "scl/math/array.h"
#include "scl/math/lagrange.h"
#include "scl/math/vector.h"
#include "scl/primitives/prg.h"
#include "scl/ss/shamir.h"

namespace scl {

/**
 * @brief A secret share in the Pedersen VSS scheme.
 * @ingroup ss
 *
 * A degree \f$ t \f$ Pedersen VSS share of a value \f$ x \f$ is triple of
 *  values \f$ (x', r', \vec{c}) \f$ such that
 * - \f$ x' \f$ is a degree-\f$ t \f$ Shamir share of \f$ x \f$
 * - \f$ r' \f$ is a degree-\f$ t \f$ Shamir share of a random value \f$ r \f$
 * - \f$ \vec{c} \f$ is a vector of commitments to \f$ t + 1 \f$ shares.
 *
 * Note that the commitments can be used to interpolate commitments for any
 * other party's share, as well as the secret. To validate that this share is
 * correct, we need to ensure that
 * \f$
 *  x'\cdot G + r'\cdot H = interp(\vec{c}, i)
 * \f$
 * where \f$ i \f$ is the ID of this share (or party), \f$ G, H \f$ are group
 * elements, and \f$ interp \f$ is an interpolation function.
 *
 * @see pedersenVerify
 * @see computeCommitmentForIndex
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
  Array<Field, 2> share;

  /**
   * @brief The commitments.
   */
  Vector<Group> commitments;

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
 * @ingroup ss
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
  Vector<Array<Field, 2>> shares;

  /**
   * @brief The share commitments.
   */
  Vector<Group> commitments;

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
 * @ingroup ss
 */
template <typename T>
PedersenSharing<T> pedersenSecretShare(
    const typename PedersenSharing<T>::Field& secret,
    std::size_t t,
    std::size_t n,
    PRG& prg,
    const typename PedersenSharing<T>::Group& h,
    const typename PedersenSharing<T>::Field& randomness) {
  using F = typename PedersenSharing<T>::Field;
  using G = typename PedersenSharing<T>::Group;

  const Array<F, 2> s = {{secret, randomness}};
  const auto shares = createShamirSharing(s, t, n, prg);

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
 * @ingroup ss
 */
template <typename GROUP>
PedersenSharing<GROUP> pedersenSecretShare(
    const typename PedersenSharing<GROUP>::Field& secret,
    std::size_t t,
    std::size_t n,
    PRG& prg,
    const typename PedersenSharing<GROUP>::Group& h) {
  using F = typename PedersenSharing<GROUP>::Field;
  const auto rand = F::random(prg);
  return pedersenSecretShare<GROUP>(secret, t, n, prg, h, rand);
}

/**
 * @brief Compute the commitment for a particular index.
 * @ingroup ss
 */
template <typename GROUP>
GROUP computeCommitmentForIndex(const Vector<GROUP>& commitments,
                                std::size_t share_index) {
  if (share_index < commitments.size()) {
    return commitments[share_index];
  }

  using Field = typename PedersenShare<GROUP>::Field;
  using Group = typename PedersenShare<GROUP>::Group;

  const auto ns = Vector<Field>::range(commitments.size());
  const auto lb = computeLagrangeBasis(ns, share_index);
  return innerProd<Group>(lb.begin(), lb.end(), commitments.begin());
}

/**
 * @brief Verify a Pedersen secret share.
 * @ingroup ss
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
 * @ingroup ss
 */
template <typename T>
bool pedersenVerify(
    const Array<typename PedersenSharing<T>::Field, 2>& share,
    const Vector<typename PedersenSharing<T>::Group>& commitments,
    std::size_t share_index,
    const typename PedersenShare<T>::Group& h) {
  return pedersenVerify<T>({share, commitments}, share_index, h);
}

/**
 * @brief Apply a matrix to a vector of shares.
 * @ingroup ss
 *
 * This function is useful if one wishes to randomize a vector of shares using
 * e.g., a Vandermonde matrix, as in DN07.
 */
template <typename T, typename IT>
std::vector<PedersenShare<T>> apply(
    const IT begin,
    const IT end,
    const Matrix<typename PedersenShare<T>::Field>& matrix) {
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
    share_out.commitments = Vector<Group>(m);
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
 * @ingroup ss
 */
template <typename T>
std::vector<PedersenShare<T>> apply(
    const std::vector<PedersenShare<T>>& shares,
    const Matrix<typename PedersenShare<T>::Field>& matrix) {
  return apply<T>(shares.begin(), shares.end(), matrix);
}

}  // namespace scl

#endif  // SCL_SS_PEDERSEN_H
