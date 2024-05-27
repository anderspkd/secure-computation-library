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

#include <array>
#include <catch2/catch_test_macros.hpp>

#include "scl/math/curves/secp256k1.h"
#include "scl/util/hash.h"
#include "scl/util/sign.h"

using namespace scl;

TEST_CASE("ECDSA derive", "[util]") {
  auto prg = util::PRG::create("ecdsa derive");
  const auto sk = util::ECDSA::SecretKey::random(prg);
  const auto pk = util::ECDSA::derive(sk);
  REQUIRE(pk == sk * math::EC<math::ec::Secp256k1>::generator());
}

TEST_CASE("ECDSA sign", "[util]") {
  auto prg = util::PRG::create("ecdsa sign");
  const auto m = util::Hash<256>{}.update("message").finalize();
  const auto sk = util::ECDSA::SecretKey::random(prg);
  const auto sig = util::ECDSA::Sign(sk, m, prg);

  const auto pk = util::ECDSA::derive(sk);
  REQUIRE(util::ECDSA::verify(pk, sig, m));

  const std::array<unsigned char, 3> m_small = {1, 2, 3};
  const auto sig_small = util::ECDSA::Sign(sk, m_small, prg);
  REQUIRE(util::ECDSA::verify(pk, sig_small, m_small));

  REQUIRE_FALSE(util::ECDSA::verify(pk, sig_small, m));
}
