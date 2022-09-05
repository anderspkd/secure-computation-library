/**
 * @file ops_gmp_ff.cc
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

#include "./ops_gmp_ff.h"

void scl::details::ReadLimb(mp_limb_t &lmb, const unsigned char *bytes,
                            std::size_t bits_per_limbs) {
  std::size_t c = 0;
  lmb = 0;
  for (std::size_t i = 0; i < bits_per_limbs; i += 8) {
    lmb |= static_cast<mp_limb_t>(bytes[c++]) << i;
  }
}

std::size_t scl::details::FindFirstNonZero(const std::string &s) {
  int n = 0;
  for (const auto c : s) {
    if (c != '0') {
      return n;
    }
    n++;
  }
  return n;
}
