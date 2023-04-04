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

#ifndef SCL_TESTS_PROTOCOL_TRIPLE_H
#define SCL_TESTS_PROTOCOL_TRIPLE_H

#include <vector>

#include "scl/ss/additive.h"
#include "scl/util/prg.h"

namespace scl::test {

template <typename FF>
struct Triple {
  Triple(FF a, FF b, FF c) : a(a), b(b), c(c){};

  FF a;
  FF b;
  FF c;
};

template <typename FF>
std::vector<Triple<FF>> RandomTriple(util::PRG& prg) {
  auto a = FF::Random(prg);
  auto b = FF::Random(prg);
  auto c = a * b;

  auto as = ss::AdditiveShare(a, 2, prg);
  auto bs = ss::AdditiveShare(b, 2, prg);
  auto cs = ss::AdditiveShare(c, 2, prg);

  return {{as[0], bs[0], cs[0]}, {as[1], bs[1], cs[1]}};
}

template <typename FF>
std::ostream& operator<<(std::ostream& os, const Triple<FF>& triple) {
  return os << triple.a << " " << triple.b << " " << triple.c;
}

}  // namespace scl::test

#endif  // SCL_TESTS_PROTOCOL_TRIPLE_H
