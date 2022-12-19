/**
 * @file feldman.h
 *
 * SCL --- Secure Computation Library
 * Copyright (C) 2022 Anders Dalskov
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

#include "scl/math/vec.h"
#include "scl/primitives/prg.h"
#include "scl/ss/shamir.h"

namespace scl {

/**
 * @brief Class for working with Feldman secret-shares.
 *
 * <p>This class allows creation, verification and reconstruction of
 * secret-shares from Feldman's verifiable secret-sharing scheme over a suitable
 * elliptic curve.</p>
 *
 * <p>The factory is instantiated with a type \p V which should be \ref EC
 * for some curve definition. In particular, <code>V::Order</code> is assumed to
 * be an \ref FF type.</p>
 */
template <typename V>
class FeldmanSSFactory {
  /**
   * @brief The type of secret-shares.
   */
  using T = typename V::Order;

 public:
  /**
   * @brief Create a new FeldmanSSFactory object.
   * @param threshold the privacy threshold
   * @param prg a prg to use for generating randomness
   */
  static FeldmanSSFactory Create(std::size_t threshold, PRG& prg) {
    return FeldmanSSFactory(
        threshold,
        ShamirSSFactory<T>::Create(threshold, prg, SecurityLevel::PASSIVE));
  };

  /**
   * @brief The result of calling Share
   */
  struct ShareBundle {
    /**
     * @brief The secret shares.
     */
    Vec<T> shares;
    /**
     * @brief Commitments of the shares plus the secret.
     */
    Vec<V> commitments;
  };

  /**
   * @brief Secret share a value.
   * @param secret the secret to share
   * @param number_of_shares the number of shares to generate
   * @return shares and commitments of shares.
   */
  ShareBundle Share(const T& secret, std::size_t number_of_shares) const {
    auto shares = mBase.Share(secret, number_of_shares);
    return {shares, ComputeCommitments(shares)};
  };

  /**
   * @brief Check that a share is consistent with a set of commitments.
   */
  bool Verify(const T& share, const Vec<V>& commitments, int party_index) const;

  /**
   * @brief Check that a secret is consistent with a set of commitments.
   */
  bool Verify(const T& secret, const Vec<V>& commitments) const {
    return Verify(secret, commitments, -1);
  };

  /**
   * @brief Recover the value corresponding to a particular index
   * @param shares the shares to recover
   * @param index the index. Defaults to 0
   */
  T Recover(const Vec<T>& shares, int index = 0) const {
    return mBase.Recover(shares, index);
  };

  /**
   * @brief Recover a share of a particular party
   * @param shares the shares
   * @param party_index the index of a the party whose share to recover
   */
  T RecoverShare(const Vec<T>& shares, int party_index = 0) const {
    return mBase.RecoverShare(shares, party_index);
  };

 private:
  FeldmanSSFactory(std::size_t threshold, ShamirSSFactory<T> base)
      : mThreshold(threshold), mBase(base){};

  std::size_t mThreshold;
  // mutable because calling Recover on this might trigger computation of new
  // lagrange coefficients.
  mutable ShamirSSFactory<T> mBase;

  Vec<V> ComputeCommitments(const Vec<T>& shares) const;
};

template <typename V>
bool FeldmanSSFactory<V>::Verify(const T& share,
                                 const Vec<V>& commitments,
                                 int party_index) const {
  if (commitments.Size() < mThreshold + 1) {
    throw std::invalid_argument("insufficient commitments for verification");
  }
  // coefficients are indexed one-off.
  auto& coeff = mBase.GetLagrangeCoefficients(party_index + 1);
  auto v = details::UncheckedInnerProd<V>(
      coeff.begin(), coeff.end(), commitments.begin());
  return v == V::Generator() * share;
}

template <typename V>
Vec<V> FeldmanSSFactory<V>::ComputeCommitments(const Vec<T>& shares) const {
  std::vector<V> c;
  c.reserve(mThreshold + 1);
  auto gen = V::Generator();
  for (std::size_t i = 0; i < mThreshold + 1; ++i) {
    c.emplace_back(shares[i] * gen);
  }
  return Vec<V>{c};
}

}  // namespace scl

#endif  // SCL_SS_FELDMAN_H
