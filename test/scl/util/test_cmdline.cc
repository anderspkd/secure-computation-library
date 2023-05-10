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
#include <stdexcept>

#include "scl/util/cmdline.h"

using namespace scl;

#define CAPTURE_START                                               \
  std::stringstream scl_cerr_buf;                                   \
  std::stringstream scl_cout_buf;                                   \
  std::streambuf* scl_cerr = std::cerr.rdbuf(scl_cerr_buf.rdbuf()); \
  std::streambuf* scl_cout = std::cout.rdbuf(scl_cout_buf.rdbuf())

#define CAPTURE_END(output_cout, output_cerr) \
  auto(output_cout) = scl_cout_buf.str();     \
  auto(output_cerr) = scl_cerr_buf.str();     \
  std::cout.rdbuf(scl_cout);                  \
  std::cerr.rdbuf(scl_cerr)

#define WITH_EXIT_0(expr)                    \
  REQUIRE_THROWS_MATCHES((expr),             \
                         std::runtime_error, \
                         Catch::Matchers::Message("good"))

#define WITH_EXIT_1(expr)                    \
  REQUIRE_THROWS_MATCHES((expr),             \
                         std::runtime_error, \
                         Catch::Matchers::Message("bad"))

TEST_CASE("Cmdline print help", "[util]") {
  const char* argv[] = {"program", "-help"};

  auto p = util::ProgramOptions::Parser("Program description.")
               .Add(util::ProgramArg::Optional("x", "y", "default"))
               .Add(util::ProgramArg::Required("a", "b", "arg description"))
               .Add(util::ProgramFlag("w", "flag description"));

  CAPTURE_START;

  WITH_EXIT_0(p.Parse(2, (char**)argv));

  CAPTURE_END(outc, oute);

  REQUIRE(oute.empty());

  REQUIRE_THAT(outc, Catch::Matchers::StartsWith("Usage: program"));
  REQUIRE_THAT(outc, Catch::Matchers::Contains("Program description."));
  REQUIRE_THAT(outc, Catch::Matchers::Contains("-x 'y'"));
  REQUIRE_THAT(outc, Catch::Matchers::Contains("-a 'b'"));
  REQUIRE_THAT(outc, Catch::Matchers::Contains("-w"));
  REQUIRE_THAT(outc, Catch::Matchers::Contains("arg description."));
  REQUIRE_THAT(outc, Catch::Matchers::Contains("flag description."));
  REQUIRE_THAT(outc, Catch::Matchers::Contains("[default=default]"));
}

TEST_CASE("Cmdline parse with error", "[util]") {
  const char* argv[] = {"program", "-x"};

  auto p = util::ProgramOptions::Parser{};

  CAPTURE_START;
  WITH_EXIT_1(p.Parse(2, (char**)argv));
  CAPTURE_END(outc, oute);

  REQUIRE_THAT(outc, Catch::Matchers::StartsWith("Usage: program"));
  REQUIRE(oute == "ERROR: encountered unknown argument\n");
}

TEST_CASE("Cmdline parse missing required", "[util]") {
  const char* argv[] = {"program"};
  auto p =
      util::ProgramOptions::Parser{}.Add(util::ProgramArg::Required("x", "y"));

  CAPTURE_START;
  WITH_EXIT_1(p.Parse(1, (char**)argv));
  CAPTURE_END(outc, oute);

  REQUIRE(oute == "ERROR: missing required argument\n");
}

TEST_CASE("Cmdline parse invalid argument", "[util]") {
  const char* argv[] = {"program", "-x"};
  auto p =
      util::ProgramOptions::Parser{}.Add(util::ProgramArg::Required("x", "y"));

  CAPTURE_START;
  WITH_EXIT_1(p.Parse(2, (char**)argv));
  CAPTURE_END(outc, oute);

  REQUIRE(oute == "ERROR: invalid argument\n");
}

TEST_CASE("Cmdline parse invalid argument name", "[util]") {
  const char* argv[] = {"program", "x"};
  auto p =
      util::ProgramOptions::Parser{}.Add(util::ProgramArg::Required("x", "y"));

  CAPTURE_START;
  WITH_EXIT_1(p.Parse(2, (char**)argv));
  CAPTURE_END(outc, oute);

  REQUIRE(oute == "ERROR: argument must begin with '-'\n");
}

TEST_CASE("Cmdline duplicate arg definition", "[util]") {
  auto p = util::ProgramOptions::Parser{}.Add(
      util::ProgramArg::Required("x", "int"));

  CAPTURE_START;
  WITH_EXIT_1(p.Add(util::ProgramArg::Required("x", "int")));
  CAPTURE_END(outc, oute);

  REQUIRE(oute == "ERROR: duplicate argument definition\n");
}

TEST_CASE("Cmdline duplicate flag definition", "[util]") {
  auto p = util::ProgramOptions::Parser{}.Add(util::ProgramFlag("x"));

  CAPTURE_START;
  WITH_EXIT_1(p.Add(util::ProgramFlag("x")));
  CAPTURE_END(outc, oute);

  REQUIRE(oute == "ERROR: duplicate argument definition\n");
}

TEST_CASE("Cmdline parse duplicate arg", "[misc]") {
  const char* argv[] = {"program", "-x", "1", "-x", "2"};
  auto p = util::ProgramOptions::Parser{}
               .Add(util::ProgramArg::Required("x", "int"))
               .Parse(5, (char**)argv);
  REQUIRE(p.Get("x") == "2");
}

TEST_CASE("Cmdline arg", "[util]") {
  const char* argv[] = {"program", "-x", "100", "-w", "600", "-b", "true"};
  auto p = util::ProgramOptions::Parser{}
               .Add(util::ProgramArg::Required("x", "int"))
               .Add(util::ProgramArg::Required("w", "ulong"))
               .Add(util::ProgramArg::Required("b", "bool"))
               .Add(util::ProgramArg::Optional("y", "long", "100"))
               .Parse(7, (char**)argv);

  REQUIRE(p.Has("x"));
  auto v = p.Get("x");
  REQUIRE(v == "100");
  auto w = p.Get<int>("x");
  REQUIRE(w == 100);

  REQUIRE(p.Has("w"));
  auto ww = p.Get<std::size_t>("w");
  REQUIRE(ww == 600);

  REQUIRE(p.Has("b"));
  REQUIRE(p.Get<bool>("b"));

  REQUIRE(p.Has("y"));
  REQUIRE(p.Get<int>("y") == 100);
}

TEST_CASE("Cmdline flag", "[util]") {
  const char* argv[] = {"program", "-f"};
  auto p = util::ProgramOptions::Parser{}
               .Add(util::ProgramFlag("f"))
               .Add(util::ProgramFlag("h"))
               .Parse(2, (char**)argv);

  REQUIRE(p.FlagSet("f"));
  REQUIRE_FALSE(p.FlagSet("h"));
  REQUIRE_FALSE(p.FlagSet("g"));
}
