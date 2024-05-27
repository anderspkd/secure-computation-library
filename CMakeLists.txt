# SCL --- Secure Computation Library
# Copyright (C) 2024 Anders Dalskov
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.5)

project(scl VERSION 0.1.0 DESCRIPTION "Secure Computation Library" LANGUAGES CXX)

option(SCL_BUILD_TESTS
  "Build tests for SCL"
  ON)

option(
  SCL_BUILD_TEST_WITH_COVERAGE
  "Build test with coverage. Implies SCL_BUILD_TESTS=ON"
  OFF)

option(
  SCL_BUILD_WITH_ADDRESS_SANITIZATION
  "Buid SCL with -fsanitize=address. Implies SCL_BUILD_TESTS=ON"
  OFF)

option(
  SCL_BUILD_DOCUMENTATION
  "Build documentation for SCL"
  ON)

# This option is a only really here because of a github action. It might be
# better to remove it and fix the action.
option(
  SCL_DOXYGEN_BIN
  "Optional location of doxygen binary"
  "")

option(
  SCL_EXPORT_COMPILE_COMMANDS
  "Generate compile_commands.json"
  ON)

if(SCL_BUILD_TEST_WITH_COVERAGE)
  set(SCL_BUILD_TESTS ON)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

find_library(GMP gmp libgmp REQUIRED)

set(SCL_HEADERS "${CMAKE_SOURCE_DIR}/include")
set(SCL_SOURCE_FILES
  src/scl/util/str.cc
  src/scl/util/prg.cc
  src/scl/util/sha3.cc
  src/scl/util/sha256.cc
  src/scl/util/cmdline.cc
  src/scl/util/measurement.cc

  src/scl/math/fields/ff_ops_gmp.cc
  src/scl/math/fields/mersenne61.cc
  src/scl/math/fields/mersenne127.cc
  src/scl/math/fields/secp256k1_field.cc
  src/scl/math/fields/secp256k1_scalar.cc
  src/scl/math/curves/secp256k1_curve.cc
  src/scl/math/number.cc

  src/scl/coro/runtime.cc

  src/scl/net/config.cc
  src/scl/net/network.cc

  src/scl/simulation/event.cc
  src/scl/simulation/context.cc
  src/scl/simulation/config.cc
  src/scl/simulation/transport.cc
  src/scl/simulation/channel.cc
  src/scl/simulation/simulator.cc
  src/scl/simulation/runtime.cc
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ${SCL_EXPORT_COMPILE_COMMANDS})

set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD 20)

add_library(scl STATIC ${SCL_SOURCE_FILES})
target_include_directories(scl PUBLIC "${SCL_HEADERS}")
target_compile_options(scl PUBLIC "-march=native")
target_compile_options(scl PUBLIC "-Wall")
target_compile_options(scl PUBLIC "-Wextra")
target_compile_options(scl PUBLIC "-pedantic")

## indicates that SCL is being built with some extra flags that will
## produce a non-optimal build.
set(SCL_SPECIAL_BUILD OFF)

if(SCL_BUILD_WITH_ADDRESS_SANITIZATION)
  target_compile_options(scl PUBLIC "-fsanitize=address")
  target_link_libraries(scl PUBLIC "-fsanitize=address")
  set(SCL_SPECIAL_BUILD ON)
endif()

if(SCL_BUILD_TEST_WITH_COVERAGE)
  target_compile_options(scl PUBLIC "-O0")
  target_compile_options(scl PUBLIC "-g")
  target_compile_options(scl PUBLIC "--coverage")
  target_compile_options(scl PUBLIC "-fno-inline")
  target_link_libraries(scl PUBLIC gcov)
  set(SCL_SPECIAL_BUILD ON)
endif()

if (SCL_SPECIAL_BUILD MATCHES OFF)
  target_compile_options(scl PRIVATE "-O3")

  set_target_properties(scl
    PROPERTIES VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

  ## only make installation possible if build is not special.
  install(TARGETS scl
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib)

  install(DIRECTORY "${SCL_HEADERS}/"
    DESTINATION include
    FILES_MATCHING PATTERN "*.h")

endif()

if(SCL_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

if(SCL_BUILD_DOCUMENTATION)
  add_subdirectory(doc)
endif()
