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

#ifndef SCL_TESTS_PROTOCOL_TRIPLE_H
#define SCL_TESTS_PROTOCOL_TRIPLE_H

#include <vector>

#include "scl/ss/additive.h"
#include "scl/util/prg.h"

namespace scl::test {

template <typename FIELD>
struct Triple {
  Triple(FIELD a, FIELD b, FIELD c) : a(a), b(b), c(c){};

  FIELD a;
  FIELD b;
  FIELD c;
};

template <typename FIELD>
std::vector<Triple<FIELD>> randomTriple2(util::PRG& prg) {
  auto a = FIELD::random(prg);
  auto b = FIELD::random(prg);
  auto c = a * b;

  auto as = ss::additiveShare(a, 2, prg);
  auto bs = ss::additiveShare(b, 2, prg);
  auto cs = ss::additiveShare(c, 2, prg);

  return {{as[0], bs[0], cs[0]}, {as[1], bs[1], cs[1]}};
}

template <typename FIELD>
std::ostream& operator<<(std::ostream& os, const Triple<FIELD>& triple) {
  return os << triple.a << " " << triple.b << " " << triple.c;
}

}  // namespace scl::test

#endif  // SCL_TESTS_PROTOCOL_TRIPLE_H
