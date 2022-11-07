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
#include "scl/prg.h"
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
auto ReconstructShamirRobust(const Vec<T>& shares, const Vec<T>& alphas,
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

/**
 * @brief Defines some common security levels.
 *
 * These security levels are used to derive some common defaults. For example,
 * SecurityLevel::DETECT is used to indicate that ShamirSSFactory, when
 * instantiated with a threshold of \f$t\f$, should generate \f$2t + 1\f$ shares
 * if no other value is specified.
 */
enum class SecurityLevel {
  /**
   * @brief \f$t + 1\f$.
   */
  PASSIVE,

  /**
   * @brief \f$2t + 1\f$. Enough shares to detect errors.
   */
  DETECT,

  /**
   * @brief \f$3t + 1\f$. Enough shares to correct errors.
   */
  CORRECT
};

/**
 * @brief Class for reconstructing secrets from Shamir secret-shares.
 *
 * <p>The main purpose of this class is to cache the lagrange coefficients used
 * when performing interpolation. These coefficients are computed when they are
 * first needed. The only exception is the coefficients needed to reconstruct
 * the point at 0, which is usually where the secret is placed.</p>
 *
 * <p>A Reconstructor contains essentially two methods for reconstructing:
 * <ul>
 * <li>Reconstruct reconstructs a particular evaluation of the implied
 * polynomial.</li>
 * <li>ReconstructShare reconstructs a particular share of a party</li>
 * </ul>
 * Both methods do effectively the same, and the only difference is how the
 * index passed to them is interpreted. For the first, the index is interpreted
 * as-is. I.e., if we pass an index of 3, then it will recover the point
 * \f$f(3)\f$, where \f$f\f$ is the polynomial implied by the shares we provide.
 * For the latter method, passing an index of \f$i\f$ implies that we wish to
 * interpolate the point \f$f(i + 1)\f$. This is because we assume parties are
 * indexed from 0, but their evaluation indices are indexed from 1 (because the
 * 0'th index is reserved for the secret).</p>
 */
template <typename T>
class Reconstructor {
 public:
  /**
   * @brief Create a new Reconstructor instance.
   *
   * This creator method creates a new instance and pre-computes the
   * coefficients needed to recover a secret from a set of shamir shares. The
   * SecurityLevel given is used to infer the default reconstruction method:
   * <ul>
   * <li>SecurityLevel::PASSIVE: use passive reconstruction. I.e., reconstruct
   * the value we ask for, using the minimal amount of \f$t + 1\f$ shares
   * needed.</li>
   *
   * <li>SecurityLevel::DETECT: use the first \f$t+1\f$ shares to reconstruct
   * the remaning \f$t\f$ shares. If all of these check out, then reconstruct
   * the value we seek.</li>
   *
   * <li>SecurityLevel::CORRECT: use an error correcting
   * algorithm to recover the polynomial \f$f\f$ from \f$3t+1\f$ shares,
   * assuming that at most \f$t\f$ shares are faulty.</li>
   * </ul>
   *
   * @param threshold the threshold \f$t\f$
   * @param default_security_level the default security level
   */
  static Reconstructor Create(std::size_t threshold,
                              SecurityLevel default_security_level);

  /**
   * @brief Interpolate a set of shares according to a given security level
   * @param shares the shares
   * @param security_level the security level
   * @param index the index to interpolate. Defaults to 0
   */
  T Reconstruct(const Vec<T>& shares, SecurityLevel security_level,
                int index = 0) const;

  /**
   * @brief Interpolate a set of shares with the default security level
   * @param shares the shares
   * @param index the index to interpolate. Defaults to 0
   */
  T Reconstruct(const Vec<T>& shares, int index = 0) const {
    return Reconstruct(shares, mSecurityLevel, index);
  };

  /**
   * @brief Reconstruct the share of a party.
   */
  T ReconstructShare(const Vec<T>& shares, SecurityLevel level,
                     int party_index = 0) const {
    return Reconstruct(shares, level, party_index + 1);
  };

  /**
   * @brief Reconstruct the share of a party.
   */
  T ReconstructShare(const Vec<T>& shares, int party_index = 0) const {
    return Reconstruct(shares, party_index + 1);
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

 private:
  Reconstructor(std::size_t threshold, SecurityLevel security_level)
      : mThreshold(threshold), mSecurityLevel(security_level){};

  void ComputeLagrangeCoefficients(int index) const;

  std::size_t mThreshold;
  SecurityLevel mSecurityLevel;
  mutable std::unordered_map<int, Vec<T>> mLagrangeCoeff;
};

template <typename T>
Reconstructor<T> Reconstructor<T>::Create(
    std::size_t threshold, SecurityLevel default_security_level) {
  Reconstructor<T> intr(threshold, default_security_level);
  intr.ComputeLagrangeCoefficients(0);
  return intr;
}

template <typename T>
T Reconstructor<T>::Reconstruct(const Vec<T>& shares,
                                SecurityLevel security_level, int index) const {
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
void Reconstructor<T>::ComputeLagrangeCoefficients(int index) const {
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

/**
 * @brief A factory object for creating Shamir secret shares.
 * @tparam T the finite field to use.
 */
template <typename T>
class ShamirSSFactory {
 public:
  /**
   * @brief Create a new Shamir secret share factory
   * @param t the privacy threshold to use
   * @param prg the PRG to use when generating new shares
   * @param security_level the SecurityLevel to use. Default to
   * SecurityLevel::DETECT
   */
  ShamirSSFactory(std::size_t t, PRG& prg,
                  SecurityLevel security_level = SecurityLevel::DETECT)
      : mThreshold(t), mPrg(prg), mDefaultSecurityLevel(security_level){};

  /**
   * @brief Create a new set of Shamir secret shares of a secret
   * @param secret the secret
   * @param number_of_shares the number of shares to generate. If empty, then
   * the number of shares is determined by GetDefaultNumberOfShares
   * @return a vector of shares.
   */
  Vec<T> Share(const T& secret,
               std::optional<std::size_t> number_of_shares = {});

  /**
   * @brief Get the number of shares to create based on set security level
   *
   * The returned number is determined based on the \ref SecurityLevel
   * provided during construction. Given a threshold \f$t\f$, then the number of
   * shares is computed based on the following rules:
   */
  std::size_t GetDefaultNumberOfShares() const {
    switch (mDefaultSecurityLevel) {
      case SecurityLevel::PASSIVE:
        return mThreshold + 1;
      case SecurityLevel::DETECT:
        return 2 * mThreshold + 1;
      default:  // SecurityLevel::ROBUST:
        return 3 * mThreshold + 1;
    }
  };

  /**
   * @brief Get an scl::Interpolator suitable for this factory.
   */
  Reconstructor<T> GetInterpolator() const {
    return Reconstructor<T>::Create(mThreshold, mDefaultSecurityLevel);
  };

 private:
  std::size_t mThreshold;
  PRG mPrg;
  SecurityLevel mDefaultSecurityLevel;
};

template <typename T>
Vec<T> ShamirSSFactory<T>::Share(const T& secret,
                                 std::optional<std::size_t> number_of_shares) {
  auto coeff = Vec<T>::PartialRandom(
      mThreshold + 1, [](std::size_t i) { return i > 0; }, mPrg);
  coeff[0] = secret;
  auto p = details::Polynomial<T>::Create(coeff);

  auto n = number_of_shares.value_or(GetDefaultNumberOfShares());
  std::vector<T> shares;
  shares.reserve(n);
  for (std::size_t i = 0; i < n; ++i) {
    shares.emplace_back(p.Evaluate(T(i + 1)));
  }
  return Vec<T>(shares);
}

}  // namespace details
}  // namespace scl

#endif  // SCL_SS_SHAMIR_H
