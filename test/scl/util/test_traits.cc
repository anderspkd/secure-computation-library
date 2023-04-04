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

#include <catch2/catch.hpp>

#include "scl/math/fp.h"
#include "scl/math/vec.h"
#include "scl/util/traits.h"

using namespace scl;

TEST_CASE("Traits", "[util]") {
  using FF = math::Fp<61>;

  REQUIRE(util::Serializable<FF>::value == true);
  REQUIRE(util::Serializable<int>::value == false);
  // not serializable because Vec's Read/Write methods do not have the right
  // signature.
  REQUIRE(util::Serializable<math::Vec<FF>>::value == false);
}
