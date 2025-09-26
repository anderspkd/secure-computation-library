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

#ifndef SCL_SS_ADDITIVE_H
#define SCL_SS_ADDITIVE_H

#include "scl/math/vector.h"
#include "scl/primitives/prg.h"

namespace scl {

/**
 * @brief Creates an additive secret-sharing.
 * @param secret the secret to secret-share.
 * @param n the number of shares.
 * @param prg a PRG used to generate random shares.
 * @return An additive secret-sharing.
 *
 * <p>An additive secret-sharing of a value \f$x\f$ is a list
 * \f$(x_1,x_2,\dots,x_n)\f$ of values such that \f$x=\sum_i x_i\f$.
 *
 * <p>An additive secret-sharing output by this function is a math::Vec object,
 * and so reconstructing the secret is simply <code>shares.sum()</code>.
 */
template <typename T>
Vector<T> additiveShare(const T& secret, std::size_t n, PRG& prg) {
  std::vector<T> shares;
  shares.reserve(n);
  auto sum = T::zero();
  for (std::size_t i = 0; i < n - 1; ++i) {
    const auto s = T::random(prg);
    shares.emplace_back(s);
    sum += s;
  }
  shares.emplace_back(secret - sum);
  return shares;
}

}  // namespace scl

#endif  // SCL_SS_ADDITIVE_H
