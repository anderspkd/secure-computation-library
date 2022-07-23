/**
 * @file test_config.cc
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

#include <catch2/catch.hpp>
#include <iostream>
#include <stdexcept>

#include "scl/net/config.h"

TEST_CASE("Config", "[network]") {
  SECTION("From file") {
    auto filename = SCL_TEST_DATA_DIR "3_parties.txt";
    auto cfg = scl::NetworkConfig::Load(0, filename);

    REQUIRE(cfg.NetworkSize() == 3);
    REQUIRE(cfg.Id() == 0);
    auto parties = cfg.Parties();
    REQUIRE(parties[0].hostname == "1.2.3.4");
    REQUIRE(parties[0].port == 8000);
    REQUIRE(parties[1].hostname == "2.3.4.5");
    REQUIRE(parties[1].port == 5000);
    REQUIRE(parties[2].hostname == "5.5.5.5");
    REQUIRE(parties[2].port == 3000);
  }

  SECTION("Invalid file") {
    auto invalid_empty = SCL_TEST_DATA_DIR "invalid_no_entries.txt";
    REQUIRE_THROWS_MATCHES(scl::NetworkConfig::Load(0, invalid_empty),
                           std::invalid_argument,
                           Catch::Matchers::Message("n cannot be zero"));

    auto valid = SCL_TEST_DATA_DIR "3_parties.txt";
    REQUIRE_THROWS_MATCHES(scl::NetworkConfig::Load(4, valid),
                           std::invalid_argument,
                           Catch::Matchers::Message("invalid id"));

    auto invalid_entry = SCL_TEST_DATA_DIR "invalid_entry.txt";
    REQUIRE_THROWS_MATCHES(
        scl::NetworkConfig::Load(0, invalid_entry), std::invalid_argument,
        Catch::Matchers::Message("invalid entry in config file"));

    auto invalid_non_existing_file = "";
    REQUIRE_THROWS_MATCHES(
        scl::NetworkConfig::Load(0, invalid_non_existing_file),
        std::invalid_argument, Catch::Matchers::Message("could not open file"));
  }

  SECTION("All local") {
    auto cfg = scl::NetworkConfig::Localhost(0, 5);
    REQUIRE(cfg.Id() == 0);
    REQUIRE(cfg.NetworkSize() == 5);
    int i = 0;
    for (const auto& ci : cfg.Parties()) {
      REQUIRE(ci.port == DEFAULT_PORT_OFFSET + i++);
      REQUIRE(ci.hostname == "127.0.0.1");
    }
  }

  SECTION("ToString") {
    scl::NetworkConfig cfg(1, {{0, "1.2.3.4", 123}, {1, "4.4.4.4", 444}});
    std::string expected = "[id=1, {0, 1.2.3.4, 123}, {1, 4.4.4.4, 444}]";
    REQUIRE(cfg.ToString() == expected);
  }

  SECTION("Validate") {
    REQUIRE_THROWS_MATCHES(
        scl::NetworkConfig(2, {{0, "1.2.3.4", 123}, {1, "4.4.4.4", 444}}),
        std::invalid_argument,
        Catch::Matchers::Message("my ID is invalid in config"));

    REQUIRE_THROWS_MATCHES(
        scl::NetworkConfig(1, {{2, "1.2.3.4", 123}, {1, "4.4.4.4", 444}}),
        std::invalid_argument,
        Catch::Matchers::Message("invalid ID in config"));

    REQUIRE_THROWS_MATCHES(
        scl::NetworkConfig(1, {{0, "1.2.3.4", 123}, {0, "4.4.4.4", 444}}),
        std::invalid_argument,
        Catch::Matchers::Message("config has duplicate party ids"));
  }
}
