/**
 * @file shamir.h
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

#ifndef SCL_SS_SHAMIR_H
#define SCL_SS_SHAMIR_H

#include <array>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "scl/math/la.h"
#include "scl/math/vec.h"
#include "scl/primitives/prg.h"
#include "scl/ss/poly.h"

namespace scl {
namespace details {

/**
 * @brief Robust reconstruction.
 *
 * <p>This function performs a robust Shamir secret-share reconstruction. Given
 * at least \f$3t + 1\f$ pairs \f$(f(x),x)\f$, where each \f$f(x)\f$ is a share
 * and \f$x\f$ a corresponding evaluation alpha, finds the degree \f$t\f$
 * polynomial \f$f\f$ that passes through all supplied shares. </p>
 *
 * <p>The return value is a pair of polynomials \f$(f,e)\f$ where \f$f\f$ is the
 * reconstructed polynomial and \f$e\f$ a polynomial whose roots indicate which
 * (if any) of the input shares were invalid. I.e., \f$e(i)=0\f$ if
 * <code>share[i]</code> had an error.</p>
 *
 * @param shares the shares to use for reconstruction
 * @param alphas the evaluation alphas
 * @param t the degree of the polynomial to reconstruct
 * @return a pair of polynomials.
 */
template <typename T>
auto ReconstructShamirRobust(const Vec<T>& shares,
                             const Vec<T>& alphas,
                             std::size_t t) {
  std::size_t n = 3 * t + 1;
  if (n > shares.Size()) {
    throw std::invalid_argument(
        "not enough shares to reconstruct with error correction");
  }
  if (n > alphas.Size()) {
    throw std::invalid_argument(
        "not enough alphas to reconstruct with error correction");
  }

  Mat<T> A(n);
  Vec<T> b(n);
  Vec<T> x(n);
  int e;
  for (std::size_t k = 0; k <= t; ++k) {
    e = t - k;  // NOLINT

    for (std::size_t i = 0; i < n; ++i) {
      b[i] = -shares[i];
      A(i, 0) = shares[i];
      for (int j = 1; j <= e; ++j) {
        A(i, j) = A(i, j - 1) * alphas[i];
        b[i] *= alphas[i];
      }

      A(i, e) = -T(1);
      for (std::size_t j = e + 1; j < n; ++j) {
        A(i, j) = A(i, j - 1) * alphas[i];
      }
    }

    if (SolveLinearSystem(x, A, b)) {
      break;
    }
  }

  Vec<T> cE{x.begin(), x.begin() + e + 1};
  cE[e] = T(1);

  auto E = details::Polynomial<T>::Create(cE);
  auto Q = details::Polynomial<T>::Create(Vec<T>{x.begin() + e, x.end()});
  auto qr = Q.Divide(E);

  if (!qr[1].IsZero()) {
    throw std::logic_error("could not correct shares");
  }

  return std::make_pair(qr[0], E);
}

/**
 * @brief Robust reconstruction.
 *
 * This function is a short-hand for <code>ReconstructShamirRobust(shares,
 * Vec<T>::Range(1, shares.Size() + 1), t)</code>.
 *
 * @param shares the shares
 * @param t the degree of the polynomial to reconstruct
 * @return \f$f(0)\f$ where \f$f\f$ is the reconstructed polynomial.
 */
template <typename T>
T ReconstructShamirRobust(const Vec<T>& shares, std::size_t t) {
  auto p =
      ReconstructShamirRobust(shares, Vec<T>::Range(1, shares.Size() + 1), t);
  return std::get<0>(p).Evaluate(T(0));
}
}  // namespace details

/**
 * @brief Defines some commonly used security levels for Shamir secret-sharing.
 *
 * The SecurityLevel enum describes different threat models that Shamir
 * secret-sharing is typically used with. Each value should be interpreted
 * relative to an implicit threshold \f$t\f$.
 */
enum class SecurityLevel {
  /**
   * @brief Passive security.
   *
   * This level describes a passive adversary threat model. When this level is
   * used, only \f$t+1\f$ shares are created, and secrets are recovered with
   * standard Lagrange interpolation without any form of checks.
   */
  PASSIVE,

  /**
   * @brief Active security with error detection.
   *
   * This level describes an active adversary threat model. When this level is
   * used, \f$n = 2t + 1\f$ shares are created, and secrets are recovered by
   * using the first \f$t + 1\f$ shares to recover the remaining \f$n - t\f$
   * shares, which are checked for equality. If this check fails, then
   * reconstruction could not proceed. Otherwise the secret is recovered as in
   * the passive case (in which case the secret is guaranteed to be the right
   * one).
   */
  DETECT,

  /**
   * @brief Active security with error correction.
   *
   * This level describes an active adversary threat model. When this level is
   * used, \f$3t+1\f$ shares are generated. Recovery proceeds via. the
   * Berelkamp-Welch error correction algorithm and guarantees that the right
   * secret is recovered, provided the input shares contain at most \f$t\f$
   * errors.
   */
  CORRECT
};

/**
 * @brief Class for working with Shamir secret-shares.
 *
 * <p>This class can be used to create Shamir secret-shares of provided secrets,
 * and to recover secrets from Shamir secret-shares. On creation, this class
 * takes a threshold \f$t\f$ and a security level. The security level is used to
 * determine (1) how many shares to create when secret-sharing a secret, and (2)
 * how to recover a secret from a set of secret-shares. For example, if
 * SecurityLevel::DETECT is passed as the security level and \f$t = 3\f$, then
 * the factory object will create \f$2t + 1 = 7\f$ shares when secret-sharing
 * something, and reconstruction uses an algorithm which detects errors in the
 * provided shares (i.e., reconstruction in this case, if it succeeds,
 * guarantees that the recovered secret is the right one).</p>
 *
 * @tparam T the share type.
 * @see SecurityLevel
 */
template <typename T>
class ShamirSSFactory {
 public:
  /**
   * @brief Create a new object for working with Shamir secret-shares.
   * @param threshold the threshold \f$t\f$
   * @param prg the prg
   * @param default_security_level the default security level
   */
  static ShamirSSFactory Create(std::size_t threshold,
                                PRG& prg,
                                SecurityLevel default_security_level);

  /**
   * @brief Create a Shamir secret-sharing of a secret.
   * @param secret the secret
   * @param number_of_shares the number of shares to generate. If empty, then
   * the number of shares is determined by GetDefaultNumberOfShares
   * @return a vector of shares.
   */
  Vec<T> Share(const T& secret,
               std::optional<std::size_t> number_of_shares = {});

  /**
   * @brief Recover a secret from a vector of secret-shares.
   *
   * This method technically recovers a particular coefficient in the original
   * polynomial used to generate the input shares. By default, the coefficient
   * to recover is the constant term, which is where the secret lies. However,
   * this method can also be used to recover the share of another party
   * (although, \ref RecoverShare has a clearer syntax).
   *
   * @param shares the shares
   * @param security_level the security level to use
   * @param index the index to interpolate. Defaults to 0
   * @return the recovered secret.
   * @throws std::invalid_argument if not enough shares is given for the
   * provided security level.
   * @throws std::logic_error if reconstruction fails (depends on the security
   * level).
   */
  T Recover(const Vec<T>& shares,
            SecurityLevel security_level,
            int index = 0) const;

  /**
   * @brief Recover a secret from a vector of secret-shares.
   *
   * This works like \ref Recover with the security level set to the one
   * passed during creation of the factory.
   *
   * @param shares the shares
   * @param index the index to interpolate. Defaults to 0
   */
  T Recover(const Vec<T>& shares, int index = 0) const {
    return Recover(shares, mSecurityLevel, index);
  };

  /**
   * @brief Recover the share of a particular party.
   *
   * Because parties' indices are counted from 1 when creating the
   * secret-shares, this method is simply a shorthand for
   * <code>Reconstruct(shares, level, party_index + 1)</code>.
   *
   * @param shares the shares
   * @param level the security level to use
   * @param party_index the index of a party
   * @return the recovered secret.
   */
  T RecoverShare(const Vec<T>& shares,
                 SecurityLevel level,
                 int party_index = 0) const {
    return Recover(shares, level, party_index + 1);
  };

  /**
   * @brief Recover the share of a party.
   */
  T RecoverShare(const Vec<T>& shares, int party_index = 0) const {
    return RecoverShare(shares, mSecurityLevel, party_index);
  };

  /**
   * @brief Get the lagrange coefficients for interpolating a particular index.
   *
   * This method modifies the internal cache in case coefficients for the
   * requested index does not exist.
   */
  const Vec<T>& GetLagrangeCoefficients(int index) const {
    if (mLagrangeCoeff.count(index) == 0) {
      ComputeLagrangeCoefficients(index);
    }
    return mLagrangeCoeff[index];
  };

  /**
   * @brief Get the number of shares to create based on set security level
   *
   * The returned number is determined based on the \ref scl::SecurityLevel
   * provided during construction. Given a threshold \f$t\f$, then the number of
   * shares is computed based on the following rules:
   */
  std::size_t GetDefaultNumberOfShares() const {
    switch (mSecurityLevel) {
      case SecurityLevel::PASSIVE:
        return mThreshold + 1;
      case SecurityLevel::DETECT:
        return 2 * mThreshold + 1;
      default:  // SecurityLevel::ROBUST:
        return 3 * mThreshold + 1;
    }
  };

 private:
  ShamirSSFactory(std::size_t threshold, PRG& prg, SecurityLevel security_level)
      : mThreshold(threshold), mPrg(prg), mSecurityLevel(security_level){};

  void ComputeLagrangeCoefficients(int index) const;

  std::size_t mThreshold;
  PRG mPrg;
  SecurityLevel mSecurityLevel;
  mutable std::unordered_map<int, Vec<T>> mLagrangeCoeff;
};

template <typename T>
ShamirSSFactory<T> ShamirSSFactory<T>::Create(
    std::size_t threshold, PRG& prg, SecurityLevel default_security_level) {
  ShamirSSFactory<T> ssf(threshold, prg, default_security_level);
  ssf.ComputeLagrangeCoefficients(0);
  return ssf;
}

template <typename T>
T ShamirSSFactory<T>::Recover(const Vec<T>& shares,
                              SecurityLevel security_level,
                              int index) const {
  if (security_level == SecurityLevel::CORRECT) {
    // TODO(anders): Currently only supports index = 0
    return details::ReconstructShamirRobust(shares, mThreshold);
  }

  const auto min_size = mThreshold + 1;
  if (shares.Size() < min_size) {
    throw std::invalid_argument("not enough shares to reconstruct");
  }

  const auto& coeff = GetLagrangeCoefficients(index);
  auto x = details::UncheckedInnerProd<T>(
      shares.begin(), shares.begin() + min_size, coeff.begin());

  if (security_level == SecurityLevel::DETECT) {
    const auto n = 2 * mThreshold + 1;
    if (shares.Size() < n) {
      throw std::invalid_argument("not enough shares to detect errors");
    }

    for (std::size_t i = min_size; i < n; ++i) {
      const auto& ccoeff = GetLagrangeCoefficients(i + 1);
      const auto c = details::UncheckedInnerProd<T>(
          shares.begin(), shares.begin() + min_size, ccoeff.begin());
      if (c != shares[i]) {
        throw std::logic_error("error detected during reconstruction");
      }
    }
  }
  return x;
}

template <typename T>
void ShamirSSFactory<T>::ComputeLagrangeCoefficients(int index) const {
  Vec<T> coeff(mThreshold + 1);
  const auto x = T(index);
  for (std::size_t j = 0; j <= mThreshold; ++j) {
    auto ell = T::One();
    const auto xj = T(j + 1);
    for (std::size_t m = 0; m <= mThreshold; ++m) {
      if (j != m) {
        const auto xm = T(m + 1);
        ell *= (x - xm) / (xj - xm);
      }
    }
    coeff[j] = ell;
  }
  mLagrangeCoeff[index] = coeff;
}

namespace details {

/**
 * @brief Create a polynomial for Shamir SS.
 * @param secret the secret
 * @param prg a PRG to use for generating the coefficients
 * @param degree the degree of the polynomial
 * @return a degree \p degree polynomial.
 */
template <typename T>
Polynomial<T> CreateSharePolynomial(const T& secret,
                                    PRG& prg,
                                    std::size_t degree) {
  auto coeff = Vec<T>::PartialRandom(
      degree + 1, [](std::size_t i) { return i > 0; }, prg);
  coeff[0] = secret;
  return Polynomial<T>::Create(coeff);
}

}  // namespace details

template <typename T>
Vec<T> ShamirSSFactory<T>::Share(const T& secret,
                                 std::optional<std::size_t> number_of_shares) {
  auto p = details::CreateSharePolynomial(secret, mPrg, mThreshold);
  auto n = number_of_shares.value_or(GetDefaultNumberOfShares());
  std::vector<T> shares;
  shares.reserve(n);
  for (std::size_t i = 0; i < n; ++i) {
    shares.emplace_back(p.Evaluate(T(i + 1)));
  }
  return Vec<T>(shares);
}

}  // namespace scl

#endif  // SCL_SS_SHAMIR_H
