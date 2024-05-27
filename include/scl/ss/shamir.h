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

#ifndef SCL_SS_SHAMIR_H
#define SCL_SS_SHAMIR_H

#include <array>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "scl/math/lagrange.h"
#include "scl/math/matrix.h"
#include "scl/math/poly.h"
#include "scl/math/vector.h"
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
math::Vector<T> shamirSecretShare(const T& secret,
                                  std::size_t t,
                                  std::size_t n,
                                  util::PRG& prg) {
  auto c = math::Vector<T>::random(t + 1, prg);
  c[0] = secret;
  const auto p = math::Polynomial<T>::create(c);

  std::vector<T> shares;
  shares.reserve(n);
  auto x = T::one();
  for (std::size_t i = 1; i <= n; ++i) {
    shares.emplace_back(p.evaluate(x++));
  }

  return math::Vector<T>(shares);
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
T shamirRecoverP(const math::Vector<T>& shares,
                 const math::Vector<T>& alphas,
                 const T& x) {
  const auto lb = math::computeLagrangeBasis(alphas, x);
  return math::innerProd<T>(shares.begin(), shares.end(), lb.begin());
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
T shamirRecoverP(const math::Vector<T>& shares) {
  return shamirRecoverP(shares,
                        math::Vector<T>::range(1, shares.size() + 1),
                        T{});
}

/**
 * @brief Recover a Shamir secret-shared secret with error detection.
 * @param shares the shares.
 * @param alphas the alphas.
 * @param t the number of shares that might contain errors.
 * @param d the degree of the sharing.
 * @param x the evaluation point.
 * @return a value.
 * @throws std::logic_error if the provided shares are not consistent.
 */
template <typename T>
T shamirRecoverD(const math::Vector<T>& shares,
                 const math::Vector<T>& alphas,
                 std::size_t t,
                 std::size_t d,
                 const T& x) {
  if (shares.size() < d + t || alphas.size() < d + t) {
    throw std::logic_error("not enough shares provided to detect errors");
  }

  const std::size_t m = d + 1;
  const auto ns = alphas.subVector(d + 1);

  for (std::size_t i = m; i < d + t; ++i) {
    auto lb = math::computeLagrangeBasis(ns, alphas[i]);
    auto yi =
        math::innerProd<T>(shares.begin(), shares.begin() + m, lb.begin());
    if (yi != shares[i]) {
      throw std::logic_error("error detected during recovery");
    }
  }

  auto lb = math::computeLagrangeBasis(ns, x);
  return math::innerProd<T>(shares.begin(), shares.begin() + m, lb.begin());
}

/**
 * @brief Recover a Shamir secret-shared secret with error detection.
 * @param shares the shares.
 * @param t the degree of the sharing.
 * @return a value.
 *
 * This function is identical to ss::ShamirRecoverD with
 * \f$\mathtt{alphas}=(1,\dots,\mathtt{shares.size()}+1)\f$ and \f$x=0\f$.
 */
template <typename T>
T shamirRecoverD(const math::Vector<T>& shares, std::size_t t) {
  const std::size_t n = 2 * t + 1;
  return shamirRecoverD(shares, math::Vector<T>::range(1, n + 1), t, t, T{});
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
ErrorCorrectedSecret<T> shamirRecoverC(const math::Vector<T>& shares,
                                       const math::Vector<T>& alphas) {
  const std::size_t t = (shares.size() - 1) / 3;
  const std::size_t n = 3 * t + 1;

  math::Matrix<T> A(n);
  math::Vector<T> b(n);
  math::Vector<T> x(n);
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

    if (solveLinearSystem(x, A, b)) {
      break;
    }
  }

  math::Vector<T> cE{x.begin(), x.begin() + e + 1};
  cE[e] = T(1);

  auto E = math::Polynomial<T>::create(cE);
  auto Q = math::Polynomial<T>::create(math::Vector<T>{x.begin() + e, x.end()});
  auto qr = Q.divide(E);

  if (!qr[1].isZero()) {
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
ErrorCorrectedSecret<T> shamirRecoverC(const math::Vector<T>& shares) {
  return shamirRecoverC(shares, math::Vector<T>::range(1, shares.size() + 1));
}

}  // namespace scl::ss

#endif  // SCL_SS_SHAMIR_H
