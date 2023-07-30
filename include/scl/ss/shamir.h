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
 * @tparam T a finite field type.
 * @param secret the secret to secret-share.
 * @param t the privacy threshold.
 * @param n the number of shares to output.
 * @param prg a prg for creating randomness.
 * @return a Shamir secret-sharing.
 *
 * This function creates a random polynomial \f$f\f$ of degree \f$t\f$ and such
 * that \f$f(0)=\mathtt{secret}\f$. The return value is a list of evaluation
 * points (the shares) defined as \f$(f(1), f(2),\dots,f(n))\f$, where the
 * points in which \f$f\f$ is evaluated is called the alphas.
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
 * @param alphas the alphas.
 * @param x the evaluation point.
 * @return a value.
 *
 * This function interpolates a polynomial running through the points \f$(s_i,
 * \alpha_i)\f$ where \f$s_i=\mathtt{share}[i]\f$ and
 * \f$\alpha_i=\mathtt{alphas}[i]\f$ and returns \f$f(x)\f$.
 */
template <typename T>
T ShamirRecoverP(const math::Vec<T>& shares,
                 const math::Vec<T>& alphas,
                 const T& x) {
  const auto lb = math::ComputeLagrangeBasis(alphas, x);
  return math::UncheckedInnerProd<T>(shares.begin(), shares.end(), lb.begin());
}

/**
 * @brief Recover a Shamir secret-shared secret.
 * @param shares the shares.
 * @return a value.
 *
 * This function is identical to ss::ShamirRecoverP with
 * \f$\mathtt{alphas}=(1,2,\dots,\mathtt{shares.size()} + 1)\f$ and
 * \f$x=0\f$. It can be used to interpolate (with passive security) a share as
 * obtained from ss::ShamirShare.
 */
template <typename T>
T ShamirRecoverP(const math::Vec<T>& shares) {
  return ShamirRecoverP(shares,
                        math::Vec<T>::Range(1, shares.Size() + 1),
                        T::Zero());
}

/**
 * @brief Recover a Shamir secret-shared secret with error detection.
 * @param shares the shares.
 * @param alphas the alphas.
 * @param x the evaluation point.
 * @return a value.
 * @throws std::logic_error if the provided shares are not consistent.
 *
 * Let \f$n=\mathtt{shares.size()}\f$ and \f$t=(n-1)/2\f$. This function
 * interpolates a polynomial \f$f\f$ running through \f$(s_i,\alpha_i)\f$ where
 * \f$s_i=\mathtt{shares}[i]\f$, \f$\alpha_i=\mathtt{alphas}[i]\f$ for
 * \f$i=1,\dots,t\f$. Note that this implies that \f$f\f$ has degree
 * \f$t\f$. The interpolated polynomial must be consistent with the remaining
 * shares and alphas, that is \f$f(\alpha_i)=s_i\f$ for \f$i=t+1,\dots,n\f$. If
 * this is the case, then \f$f(x)\f$ is returned, otherwise an
 * <code>std::logic_error</code> is thrown.
 */
template <typename T>
T ShamirRecoverD(const math::Vec<T>& shares,
                 const math::Vec<T>& alphas,
                 const T& x) {
  const std::size_t t = (shares.Size() - 1) / 2;
  const std::size_t n = 2 * t + 1;
  const auto ns = alphas.SubVector(t + 1);

  for (std::size_t i = t + 1; i < n; ++i) {
    // Shares are indexed starting from 1.
    auto lb = math::ComputeLagrangeBasis(ns, alphas[i]);
    auto yi = math::UncheckedInnerProd<T>(shares.begin(),
                                          shares.begin() + t + 1,
                                          lb.begin());
    if (yi != shares[i]) {
      throw std::logic_error("error detected during recovery");
    }
  }

  auto lb = math::ComputeLagrangeBasis(ns, x);
  return math::UncheckedInnerProd<T>(shares.begin(),
                                     shares.begin() + t + 1,
                                     lb.begin());
}

/**
 * @brief Recover a Shamir secret-shared secret with error detection.
 * @param shares the shares.
 * @return a value.
 *
 * This function is identical to ss::ShamirRecoverD with
 * \f$\mathtt{alphas}=(1,\dots,\mathtt{shares.size()}+1)\f$ and \f$x=0\f$.
 */
template <typename T>
T ShamirRecoverD(const math::Vec<T>& shares) {
  const std::size_t t = (shares.Size() - 1) / 2;
  const std::size_t n = 2 * t + 1;
  return ShamirRecoverD(shares, math::Vec<T>::Range(1, n + 1), T::Zero());
}

/**
 * @brief The result of an error corrected Shamir sharing.
 *
 * <p>When recovering a Shamir secret-shared value with error correction, the
 * result is either two polynomials or an error, where an error only occurs when
 * too many errors are present (i.e., when correction was not possible).
 *
 * <p>When correction is possible, the result is a pair \f$(f,e)\f$ where
 * \f$f\f$ is the recovered polynomial, and in particular, \f$f(0)\f$ is the
 * value that was secret-shared in case the sharing was constructed using
 * ss::ShamirShare. The other polynomial \f$e\f$ indicates which shares were
 * bad. I.e., \f$e(\alpha_i)=0\f$ says that the evaluation point
 * \f$(s_i,\alpha_i)\f$ did not lie on the polynomial \f$f\f$. Usually,
 * \f$\alpha_i\f$ is a party identifier, so this is the same as saying that
 * party \f$P_{\alpha_i}\f$ sent an invalid share.
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
 * @param alphas the alphas.
 * @return a pair of polynomials.
 * @throws std::logic_error if error correction failed.
 *
 * <p>Let \f$n=\mathtt{shares.size()}\f$ and \f$t=(n-1)/3\f$. Given a list of
 * evaluation points \f$(s_i,\alpha_i)\f$ with \f$s_i=\mathtt{shares}[i]\f$ and
 * \f$\alpha_i=\mathtt{alphas}[i]\f$, this function attempts to recover a
 * polynomial \f$f\f$ of degree \f$t\f$. If this is possible, the recovered
 * polynomial is returned together with a polynomial indicating which supplied
 * shares did not lie on the polynomial.
 *
 * <p>This function can correct up to \f$t\f$ errors in the supplied shares.
 */
template <typename T>
ErrorCorrectedSecret<T> ShamirRecoverC(const math::Vec<T>& shares,
                                       const math::Vec<T>& alphas) {
  const std::size_t t = (shares.Size() - 1) / 3;
  const std::size_t n = 3 * t + 1;

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

/**
 * @brief Recover a Shamir secret-shared secret with error correction.
 * @param shares the shares.
 * @return a pair of polynomials.
 *
 * This function is identical to ss::ShamirRecoverC with
 * \f$\mathtt{alphas}=(1,\dots,\mathtt{shares.size()}+1)\f$.
 */
template <typename T>
ErrorCorrectedSecret<T> ShamirRecoverC(const math::Vec<T>& shares) {
  return ShamirRecoverC(shares, math::Vec<T>::Range(1, shares.Size() + 1));
}

}  // namespace scl::ss

#endif  // SCL_SS_SHAMIR_H
