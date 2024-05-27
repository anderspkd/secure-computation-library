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

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <sstream>
#include <stdexcept>

#include "scl/util/cmdline.h"

using namespace scl;

#define CAPTURE_START                                               \
  std::stringstream scl_cerr_buf;                                   \
  std::stringstream scl_cout_buf;                                   \
  std::streambuf* scl_cerr = std::cerr.rdbuf(scl_cerr_buf.rdbuf()); \
  std::streambuf* scl_cout = std::cout.rdbuf(scl_cout_buf.rdbuf())

#define CAPTURE_END(output_cout, output_cerr) \
  auto output_cout = scl_cout_buf.str();      \
  auto output_cerr = scl_cerr_buf.str();      \
  std::cout.rdbuf(scl_cout);                  \
  std::cerr.rdbuf(scl_cerr)

#define WITH_EXIT_0(expr)                    \
  REQUIRE_THROWS_MATCHES((expr),             \
                         std::runtime_error, \
                         Catch::Matchers::Message("no error"))

#define WITH_EXIT_1(expr)                    \
  REQUIRE_THROWS_MATCHES((expr),             \
                         std::runtime_error, \
                         Catch::Matchers::Message("error"))

TEST_CASE("Cmdline print help", "[util]") {
  const char* argv[] = {"program", "-help"};

  auto p = util::ProgramOptions::Parser("Program description.")
               .add(util::ProgramArg::optional("x", "y", "default"))
               .add(util::ProgramArg::required("a", "b", "arg description"))
               .add(util::ProgramFlag("w", "flag description"));

  CAPTURE_START;

  WITH_EXIT_0(p.parse(2, (char**)argv, false));

  CAPTURE_END(outc, oute);

  REQUIRE(oute.empty());
  std::string line;
  auto outcs = std::istringstream(outc);

  // Usage: program -x y -a b [options ...]
  //
  // Program description.
  //
  // Required arguments
  //  -a 'b'             arg description.
  //
  // Optional Arguments
  //  -x 'y' [default=default]
  //
  // Flags
  //  -w                 flag description.

  std::getline(outcs, line);
  REQUIRE(line == "Usage: program -a b [options ...]");
  std::getline(outcs, line);
  std::getline(outcs, line);
  REQUIRE(line == "Program description.");
  std::getline(outcs, line);
  std::getline(outcs, line);
  REQUIRE(line == "Required arguments");
  std::getline(outcs, line);
  REQUIRE(line == " -a 'b'             arg description.");
  std::getline(outcs, line);
  std::getline(outcs, line);
  REQUIRE(line == "Optional arguments");
  std::getline(outcs, line);
  REQUIRE(line == " -x 'y' [default=default]");
  std::getline(outcs, line);
  std::getline(outcs, line);
  REQUIRE(line == "Flags");
  std::getline(outcs, line);
  REQUIRE(line == " -w                 flag description.");
}

TEST_CASE("Cmdline parse with error", "[util]") {
  const char* argv[] = {"program", "-x"};

  auto p = util::ProgramOptions::Parser{};

  CAPTURE_START;
  WITH_EXIT_1(p.parse(2, (char**)argv, false));
  CAPTURE_END(outc, oute);

  REQUIRE_THAT(outc, Catch::Matchers::StartsWith("Usage: program"));
  REQUIRE(oute == "ERROR: encountered unknown argument\n");
}

TEST_CASE("Cmdline parse missing required", "[util]") {
  const char* argv[] = {"program"};
  auto p =
      util::ProgramOptions::Parser{}.add(util::ProgramArg::required("x", "y"));

  CAPTURE_START;
  WITH_EXIT_1(p.parse(1, (char**)argv, false));
  CAPTURE_END(outc, oute);

  REQUIRE(oute == "ERROR: missing required argument\n");
}

TEST_CASE("Cmdline parse invalid argument", "[util]") {
  const char* argv[] = {"program", "-x"};
  auto p =
      util::ProgramOptions::Parser{}.add(util::ProgramArg::required("x", "y"));

  CAPTURE_START;
  WITH_EXIT_1(p.parse(2, (char**)argv, false));
  CAPTURE_END(outc, oute);

  REQUIRE(oute == "ERROR: invalid argument\n");
}

TEST_CASE("Cmdline parse invalid argument name", "[util]") {
  const char* argv[] = {"program", "x"};
  auto p =
      util::ProgramOptions::Parser{}.add(util::ProgramArg::required("x", "y"));

  CAPTURE_START;
  WITH_EXIT_1(p.parse(2, (char**)argv, false));
  CAPTURE_END(outc, oute);

  REQUIRE(oute == "ERROR: argument must begin with '-'\n");
}

TEST_CASE("Cmdline duplicate arg definition", "[util]") {
  auto p = util::ProgramOptions::Parser{}
               .add(util::ProgramArg::required("x", "int"))
               .add(util::ProgramArg::required("x", "int"));

  const char* argv[] = {"program", "-x", "1 "};
  CAPTURE_START;
  WITH_EXIT_1(p.parse(3, (char**)argv, false));
  CAPTURE_END(outc, oute);

  REQUIRE(oute == "ERROR: duplicate argument definition\n");
}

TEST_CASE("Cmdline duplicate flag definition", "[util]") {
  auto p = util::ProgramOptions::Parser{}
               .add(util::ProgramFlag("x"))
               .add(util::ProgramFlag("x"));

  const char* argv[] = {"program", "-x"};
  CAPTURE_START;
  WITH_EXIT_1(p.parse(2, (char**)argv, false));
  CAPTURE_END(outc, oute);

  REQUIRE(oute == "ERROR: duplicate flag definition\n");
}

TEST_CASE("Cmdline parse duplicate arg", "[misc]") {
  const char* argv[] = {"program", "-x", "1", "-x", "2"};
  auto p = util::ProgramOptions::Parser{}
               .add(util::ProgramArg::required("x", "int"))
               .parse(5, (char**)argv, false);
  REQUIRE(p.get("x") == "2");
}

TEST_CASE("Cmdline arg", "[util]") {
  const char* argv[] = {"program", "-x", "100", "-w", "600", "-b", "true"};
  auto p = util::ProgramOptions::Parser{}
               .add(util::ProgramArg::required("x", "int"))
               .add(util::ProgramArg::required("w", "ulong"))
               .add(util::ProgramArg::required("b", "bool"))
               .add(util::ProgramArg::optional("y", "long", "100"))
               .parse(7, (char**)argv, false);

  REQUIRE(p.has("x"));
  auto v = p.get("x");
  REQUIRE(v == "100");
  auto w = p.get<int>("x");
  REQUIRE(w == 100);

  REQUIRE(p.has("w"));
  auto ww = p.get<std::size_t>("w");
  REQUIRE(ww == 600);

  REQUIRE(p.has("b"));
  REQUIRE(p.get<bool>("b"));

  REQUIRE(p.has("y"));
  REQUIRE(p.get<int>("y") == 100);
}

TEST_CASE("Cmdline flag", "[util]") {
  const char* argv[] = {"program", "-f"};
  auto p = util::ProgramOptions::Parser{}
               .add(util::ProgramFlag("f"))
               .add(util::ProgramFlag("h"))
               .parse(2, (char**)argv, false);

  REQUIRE(p.flagSet("f"));
  REQUIRE_FALSE(p.flagSet("h"));
  REQUIRE_FALSE(p.flagSet("g"));
}
