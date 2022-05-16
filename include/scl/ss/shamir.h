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

#ifndef _SCL_SS_SHAMIR_H
#define _SCL_SS_SHAMIR_H

#include <array>
#include <iostream>
#include <stdexcept>

#include "scl/math/la.h"
#include "scl/math/vec.h"
#include "scl/prg.h"
#include "scl/ss/poly.h"

namespace scl {
namespace details {

/**
 * @brief Create a random polynomial suitable for create Shamir secret-shares.
 * @param secret the secret to embed in the constant term of the polynomial
 * @param t the threshold that this polynomial should support
 * @param prg a PRG used to generate random coefficients
 * @return a Polynomial that can be used to generate degree t shares.
 */
template <typename T>
details::Polynomial<T> CreateShamirSharePolynomial(const T& secret,
                                                   std::size_t t, PRG& prg) {
  if (!t) throw std::invalid_argument("threshold cannot be 0");
  auto coeff = Vec<T>::PartialRandom(
      t + 1, [](std::size_t i) { return i > 0; }, prg);
  coeff[0] = secret;
  return details::Polynomial<T>::Create(coeff);
}

/**
 * @brief Interpolate a polynomial given a list of evaluations (f(x), x).
 * @param ys the evaluation results
 * @param xs the evaluation points
 * @param k the number of points to use
 * @param x the point at which to interpolate
 * @param offset an offset into the \p ys and \p xs
 * @return f(\p x) where f was interpolated from \p k of the provided points.
 */
template <typename T>
T InterpolateAt(const Vec<T>& ys, const Vec<T>& xs, std::size_t k, const T& x,
                std::size_t offset) {
  T z;
  for (std::size_t j = 0; j < k; ++j) {
    T ell(1);
    auto xj = xs[offset + j];
    for (std::size_t m = 0; m < k; ++m) {
      if (m == j) continue;
      auto xm = xs[offset + m];
      ell *= (x - xm) / (xj - xm);
    }
    z += ys[offset + j] * ell;
  }
  return z;
}
}  // namespace details

/**
 * @brief Create a collection of shamir shares given a polynomial and points.
 * @param sharing_polynomial the polynomial used to generate the shares
 * @param alphas the evaluation points
 * @tparam T a finite field
 * @return A vector of shamir secret-shares.
 */
template <typename T>
Vec<T> CreateShamirShares(const details::Polynomial<T>& sharing_polynomial,
                          const Vec<T>& alphas) {
  auto n = alphas.Size();
  Vec<T> shares(n);
  for (std::size_t i = 0; i < n; i++) {
    shares[i] = sharing_polynomial.Evaluate(alphas[i]);
  }
  return shares;
}

/**
 * @brief Return the list of canonical evaluation points
 * @param n the number of points
 * @tparam T a finite field
 * @return the list <code>[1, 2, ..., n]</code> of finite field elements.
 */
template <typename T>
Vec<T> CanonicalAlphas(std::size_t n) {
  Vec<T> alphas(n);
  for (std::size_t i = 0; i < n; i++) alphas[i] = T{(int)(i + 1)};
  return alphas;
}

/**
 * @brief Create a shamir secret-sharing.
 * @param secret the secret
 * @param n the number of shares to create
 * @param t the privacy threshold
 * @param prg a PRG for randomness
 * @return a vector of Shamir secret-shares.
 */
template <typename T>
Vec<T> CreateShamirShares(const T& secret, std::size_t n, std::size_t t,
                          PRG& prg) {
  return CreateShamirShares(
      details::CreateShamirSharePolynomial(secret, t, prg),
      CanonicalAlphas<T>(n));
}

/**
 * @brief Reconstruct a shamir shared secret with passive security.
 * @param shares the shars
 * @param alphas the alphas
 * @param pos the position of the secret
 * @param t the threshold
 * @return the reconstructed value.
 * @throws std::invalid_argument if less than \f$t+1\f$ shares were given
 * @throws std::invalid_argument if less than \f$t+1\f$ alphas were given
 */
template <typename T>
T ReconstructShamirPassive(const Vec<T>& shares, const Vec<T>& alphas,
                           const T& pos, std::size_t t) {
  if (t + 1 > shares.Size())
    throw std::invalid_argument("not enough shares to reconstruct");
  if (t + 1 > alphas.Size())
    throw std::invalid_argument("not enough alphas to reconstruct");
  return InterpolateAt(shares, alphas, t + 1, pos, 0);
}

/**
 * @brief Reconstruct a Shamir shared secret with passive security.
 *
 * This method makes no guarantee regarding the reconstructed value. In
 * particular, this method should only be used in a passive security model.
 *
 * The shares is assumed to have been generated using alphas as output by \ref
 * scl::CanonicalAlphas, and the secret is assumed to have been placed on the
 * constant term of the sharing polynomial.
 *
 * @param shares the shares
 * @param t the threshold
 * @return the reconstructed secret.
 * @see ReconstructShamirPassive
 */
template <typename T>
T ReconstructShamirPassive(const Vec<T>& shares, std::size_t t) {
  return ReconstructShamirPassive(shares, CanonicalAlphas<T>(shares.Size()),
                                  T(0), t);
}

/**
 * @brief Reconstruct a Shamir shared secret with error detection.
 *
 * This method will attempt to reconstruct a degree \f$t\f$ shared secret from
 * at least \f$2t+1\f$ provided points. In case the provided shares do not all
 * lie on a degree \f$t\f$ polynomial, an exception is thrown.
 *
 * @param shares the shares
 * @param alphas the alphas
 * @param pos the position of the secret
 * @param t the threshold
 * @return the correct reconstructed value.
 * @throws std::invalid_argument if less than \f$2t+1\f$ shares were given
 * @throws std::invalid_argument if less than \f$2t+1\f$ alphas were given
 * @throws std::logic_error if one of the shares contained an error
 */
template <typename T>
T ReconstructShamir(const Vec<T>& shares, const Vec<T>& alphas, const T& pos,
                    std::size_t t) {
  if (2 * t + 1 > shares.Size())
    throw std::invalid_argument(
        "not enough shares to reconstruct with error detection");
  if (2 * t + 1 > alphas.Size())
    throw std::invalid_argument(
        "not enough alphas to reconstruct with error detection");

  for (std::size_t k = t + 1; k < 2 * t + 1; ++k) {
    auto s = InterpolateAt(shares, alphas, t + 1, alphas[k], 0);
    if (s != shares[k])
      throw std::logic_error("error detected during reconstruction");
  }
  return InterpolateAt(shares, alphas, t + 1, pos, 0);
}

/**
 * @brief Reconstruct a Shamir shared secret with error detection.
 * @see ReconstructShamir
 */
template <typename T>
T ReconstructShamir(const Vec<T>& shares, std::size_t t) {
  return ReconstructShamir(shares, CanonicalAlphas<T>(shares.Size()), T(0), t);
}

/**
 * @brief Reconstruct a Shamir shared secret with error correction.
 */
template <typename T>
std::array<details::Polynomial<T>, 2> ReconstructShamirRobust(
    const Vec<T>& shares, const Vec<T>& alphas, std::size_t t) {
  std::size_t n = 3 * t + 1;
  if (n > shares.Size())
    throw std::invalid_argument(
        "not enough shares to reconstruct with error correction");
  if (n > alphas.Size())
    throw std::invalid_argument(
        "not enough alphas to reconstruct with error correction");

  Mat<T> A(n);
  Vec<T> b(n);
  Vec<T> x(n);
  int e;
  for (std::size_t k = 0; k <= t; ++k) {
    e = t - k;

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

    if (SolveLinearSystem(x, A, b)) break;
  }

  Vec<T> cE{x.begin(), x.begin() + e + 1};
  cE[e] = T(1);

  auto E = details::Polynomial<T>::Create(cE);
  auto Q = details::Polynomial<T>::Create(Vec<T>{x.begin() + e, x.end()});
  auto qr = Q.Divide(E);
  if (qr[1].IsZero()) return {qr[0], E};

  throw std::logic_error("could not correct shares");
}

/**
 * @brief Reconstruct a Shamir shared secret with error correction.
 */
template <typename T>
T ReconstructShamirRobust(const Vec<T>& shares, std::size_t t) {
  auto p =
      ReconstructShamirRobust(shares, CanonicalAlphas<T>(shares.Size()), t);
  return p[0].Evaluate(T(0));
}

}  // namespace scl

#endif  // _SCL_SS_SHAMIR_H
