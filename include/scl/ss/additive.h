/**
 * @file additive.h
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

#ifndef _SCL_SS_ADDITIVE_H
#define _SCL_SS_ADDITIVE_H

#include <stdexcept>

#include "scl/math/vec.h"
#include "scl/prg.h"

namespace scl {

/**
 * @brief Creates an additive secret-sharing.
 * @param secret the secret to secret-share
 * @param n the number of shares
 * @param prg a PRG used to generate random shares
 * @return An additive secret-sharing.
 * @throws std::invalid_argument if \p n is 0.
 */
template <typename T>
Vec<T> CreateAdditiveShares(const T& secret, std::size_t n, PRG& prg) {
  if (!n) throw std::invalid_argument("cannot create shares for 0 people");

  Vec<T> shares = Vec<T>::PartialRandom(
      n, [](std::size_t i) { return i > 0; }, prg);
  shares[0] = secret - shares.Sum();
  return shares;
}

/**
 * @brief Reconstruct an additive secret-sharing.
 * @param shares the shares to reconstruct from
 * @return The reconstructed result.
 * @note this function is equivalent to <code>shares.Sum()</code>.
 */
template <typename T>
T ReconstructAdditive(const Vec<T>& shares) {
  return shares.Sum();
}

}  // namespace scl

#endif  // _SCL_SS_ADDITIVE_H
