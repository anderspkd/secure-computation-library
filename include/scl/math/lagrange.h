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

#ifndef SCL_MATH_LAGRANGE_H
#define SCL_MATH_LAGRANGE_H

#include <cstddef>
#include <unordered_map>

#include "scl/math/vec.h"

namespace scl::math {

/**
 * @brief Computes a lagrange basis for a set of nodes.
 * @param nodes the set of nodes.
 * @param x the evaluation point x.
 *
 * <p>This function computes a set of values \f$\{\ell_i(x)\}_{i<n}\f$ known as
 * the <i>Lagrange basis</i> for a polynomial of degree \f$\leq n\f$, where
 * \f$n\f$ is the number of nodes provided. This basis can be used for
 * polynomial interpolation.
 *
 * @code
 * // pairs of evaluations of some polynomial f
 * auto nodes = ...  // 0, 1, 2, 3, 4, 5
 * auto ys = ...     // f(0), f(2), f(3), f(4), f(5)
 *
 * auto basis = ComputeLagrangeBasis(nodes, 7);
 *
 * auto f7 = ys.Dot(basis);  // f(7)
 * @endcode
 *
 * <p>The nodes provided must be pairwise invertible. That is, for every
 * \f$x_i,x_j\f$ in \p nodes, then \f$x_i-x_j\f$ must be invertible for all
 * \f$i\neq j\f$.
 *
 * @see https://en.wikipedia.org/wiki/Lagrange_polynomial
 */
template <typename T>
Vec<T> ComputeLagrangeBasis(const math::Vec<T>& nodes, const T& x) {
  const auto n = nodes.Size();
  std::vector<T> b;
  b.reserve(n);
  for (std::size_t i = 0; i < n; ++i) {
    auto ell = T::One();
    const auto xi = nodes[i];
    for (std::size_t j = 0; j < n; ++j) {
      if (i != j) {
        const auto xj = nodes[j];
        ell *= (x - xj) / (xi - xj);
      }
    }
    b.emplace_back(ell);
  }
  return b;
}

/**
 * @brief Computes a lagrange basis for a set of nodes.
 * @param nodes the set of nodes.
 * @param x the evaluation point x.
 * @see ComputeLagrangeBasis
 */
template <typename T>
Vec<T> ComputeLagrangeBasis(const math::Vec<T>& nodes, int x) {
  return ComputeLagrangeBasis(nodes, T{x});
}

}  // namespace scl::math

#endif  // SCL_MATH_LAGRANGE_H
