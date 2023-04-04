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

#ifndef SCL_SS_SHAMIR_H
#define SCL_SS_SHAMIR_H

#include <array>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "scl/math/la.h"
#include "scl/math/lagrange.h"
#include "scl/math/poly.h"
#include "scl/math/vec.h"
#include "scl/util/prg.h"

namespace scl::ss {

/**
 * @brief Create a Shamir secret-sharing.
 * @param secret the secret to secret-share.
 * @param t the privacy threshold.
 * @param n the number of shares to output.
 * @param prg a prg for creating randomness.
 * @return a Shamir secret-sharing.
 */
template <typename T>
math::Vec<T> ShamirShare(const T& secret,
                         std::size_t t,
                         std::size_t n,
                         util::PRG& prg) {
  auto c = math::Vec<T>::Random(t + 1, prg);
  c[0] = secret;
  const auto p = math::Polynomial<T>::Create(c);

  std::vector<T> shares;
  shares.reserve(n);
  for (std::size_t i = 1; i <= n; ++i) {
    shares.emplace_back(p.Evaluate(T{(int)i}));
  }

  return math::Vec<T>(shares);
}

/**
 * @brief Recover a Shamir secret-shared secret.
 * @param shares the shares.
 * @return a value.
 *
 * This function interpolates the polynomial \f$f\f$ passing through all of \p
 * shares and then returns \f$f(0)\f$.
 */
template <typename T>
T ShamirRecoverP(const math::Vec<T>& shares) {
  const auto lb =
      math::ComputeLagrangeBasis(math::Vec<T>::Range(1, shares.Size() + 1), 0);
  return math::UncheckedInnerProd<T>(shares.begin(), shares.end(), lb.begin());
}

/**
 * @brief Recover a Shamir secret-shared secret with error detection.
 * @param shares the shares.
 * @return a value.
 * @throws std::logic_error if the provided shares are not consistent.
 *
 * This function attempts to interpolate a polynomial \f$f\f$ of degree
 * \f$t=(\mathtt{shares.size()}-1)/2\f$ that passes through all the provided
 * shares. If this succeeds, the \f$f(0)\f$ is returned. Otherwise an exception
 * is thrown.
 */
template <typename T>
T ShamirRecoverD(const math::Vec<T>& shares) {
  const std::size_t t = (shares.Size() - 1) / 2;
  const std::size_t n = 2 * t + 1;
  const auto ns = math::Vec<T>::Range(1, t + 2);

  for (std::size_t i = t + 1; i < n; ++i) {
    // Shares are indexed starting from 1.
    auto lb = math::ComputeLagrangeBasis(ns, i + 1);
    auto yi = math::UncheckedInnerProd<T>(shares.begin(),
                                          shares.begin() + t + 1,
                                          lb.begin());
    if (yi != shares[i]) {
      throw std::logic_error("error detected during recovery");
    }
  }

  auto lb = math::ComputeLagrangeBasis(ns, 0);
  return math::UncheckedInnerProd<T>(shares.begin(),
                                     shares.begin() + t + 1,
                                     lb.begin());
}

/**
 * @brief The result of an error corrected Shamir sharing.
 */
template <typename T>
struct ErrorCorrectedSecret {
  /**
   * @brief The recovered polynomial.
   */
  math::Polynomial<T> f;

  /**
   * @brief The error polynomial.
   */
  math::Polynomial<T> err;
};

/**
 * @brief Recover a Shamir secret-shared secret with error correction.
 * @param shares the shares.
 * @return a pair of polynomials.
 * @throws std::logic_error if error correction failed.
 */
template <typename T>
ErrorCorrectedSecret<T> ShamirRecoverC(const math::Vec<T>& shares) {
  const std::size_t t = (shares.Size() - 1) / 3;
  const std::size_t n = 3 * t + 1;
  const auto ns = math::Vec<T>::Range(1, shares.Size() + 1);

  math::Mat<T> A(n);
  math::Vec<T> b(n);
  math::Vec<T> x(n);
  int e;
  for (std::size_t k = 0; k <= t; ++k) {
    e = t - k;  // NOLINT

    for (std::size_t i = 0; i < n; ++i) {
      b[i] = -shares[i];
      A(i, 0) = shares[i];
      for (int j = 1; j <= e; ++j) {
        A(i, j) = A(i, j - 1) * ns[i];
        b[i] *= ns[i];
      }

      A(i, e) = -T(1);
      for (std::size_t j = e + 1; j < n; ++j) {
        A(i, j) = A(i, j - 1) * ns[i];
      }
    }

    if (SolveLinearSystem(x, A, b)) {
      break;
    }
  }

  math::Vec<T> cE{x.begin(), x.begin() + e + 1};
  cE[e] = T(1);

  auto E = math::Polynomial<T>::Create(cE);
  auto Q = math::Polynomial<T>::Create(math::Vec<T>{x.begin() + e, x.end()});
  auto qr = Q.Divide(E);

  if (!qr[1].IsZero()) {
    throw std::logic_error("could not correct shares");
  }

  return {qr[0], E};
}

}  // namespace scl::ss

#endif  // SCL_SS_SHAMIR_H
