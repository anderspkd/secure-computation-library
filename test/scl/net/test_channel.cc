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
#include <memory>

#include "scl/math/fp.h"
#include "scl/net/channel.h"
#include "scl/net/mem_channel.h"

using namespace scl;

TEST_CASE("Channel size limits", "[net]") {
  auto chl = net::MemoryBackedChannel::CreatePaired();
  net::Channel* c0 = chl[0].get();
  net::Channel* c1 = chl[1].get();

  const auto v = math::Vec<math::Fp<61>>::Range(0, MAX_VEC_READ_SIZE + 1);
  c0->Send(v);

  math::Vec<math::Fp<61>> vr;
  REQUIRE_THROWS_MATCHES(
      c1->Recv(vr),
      std::logic_error,
      Catch::Matchers::Message("received vector exceeds size limit"));

  const auto m = math::Mat<math::Fp<61>>::FromVector(MAX_MAT_READ_SIZE + 1,
                                                     1,
                                                     v.ToStlVector());

  c1->Send(m);

  math::Mat<math::Fp<61>> mr;
  REQUIRE_THROWS_MATCHES(
      c0->Recv(mr),
      std::logic_error,
      Catch::Matchers::Message("received matrix exceeds size limit"));
}
