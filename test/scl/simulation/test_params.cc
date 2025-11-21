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

#include <catch2/catch_test_macros.hpp>

#include "scl/simulation/params.h"

using namespace scl;

TEST_CASE("Network params", "[sim]") {
  auto np = NetworkParams::create(2);

  ChannelId cid(0, 1);

  REQUIRE(np.channel(cid).bandwidth() == np.channel(cid.flip()).bandwidth());
  REQUIRE(np.channel(cid).latency() == np.channel(cid.flip()).latency());
}
