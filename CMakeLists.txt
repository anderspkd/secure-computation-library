# SCL --- Secure Computation Library
# Copyright (C) 2025 Anders Dalskov
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

cmake_minimum_required(VERSION 3.30)

project(scl
  VERSION 0.2.0
  DESCRIPTION "Secure Computation Library"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD 20)

option(
  TESTS
  "Build tests for SCL"
  ON)

option(
  COVERAGE
  "Enable test coverage"
  OFF)

option(
  DOCS
  "Build documentation for SCL"
  ON)

option(
  ASAN
  "Enable address sanitization"
  OFF)

option(
  EXPORT_COMPILE_COMMANDS
  "Generate compile_commands.json"
  ON)

find_library(GMP gmp libgmp REQUIRED)

set(HEADERS "${CMAKE_SOURCE_DIR}/include")
set(SOURCES
  src/scl/argparser.cc
  src/scl/hex.cc

  src/scl/primitives/prg.cc
  src/scl/primitives/sha3.cc
  src/scl/primitives/sha256.cc

  src/scl/math/ff_ops_gmp.cc
  src/scl/math/mersenne61.cc
  src/scl/math/mersenne127.cc
  src/scl/math/secp256k1_field.cc
  src/scl/math/secp256k1_scalar.cc
  src/scl/math/secp256k1_curve.cc
  src/scl/math/number.cc

  src/scl/coro.cc

  src/scl/net/tcp/config.cc
  src/scl/net/tcp/utils.cc
  src/scl/net/tcp/channel.cc
  src/scl/net/tcp/network.cc

  src/scl/simulation/event.cc
  src/scl/simulation/params.cc
  src/scl/simulation/context.cc
  src/scl/simulation/transport.cc
  src/scl/simulation/channel.cc
  src/scl/simulation/simulator.cc
  src/scl/simulation/runtime.cc
)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ${EXPORT_COMPILE_COMMANDS})

add_library(scl STATIC ${SOURCES})
target_include_directories(scl PUBLIC "${HEADERS}")
target_compile_options(scl PUBLIC "-march=native")
target_compile_options(scl PUBLIC "-Wall")
target_compile_options(scl PUBLIC "-Wextra")
target_compile_options(scl PUBLIC "-pedantic")

if(ASAN)
  target_compile_options(scl PUBLIC "-fsanitize=address")
  target_link_libraries(scl PUBLIC "-fsanitize=address")
endif()

set_target_properties(scl
  PROPERTIES VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

install(TARGETS scl
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib)

install(DIRECTORY "${HEADERS}/"
  DESTINATION include
  FILES_MATCHING PATTERN "*.h")

if(TESTS)
  if(COVERAGE)
    find_program(gcov gcov REQUIRED)
    find_program(lcov lcov REQUIRED)
    find_program(genhtml genhtml REQUIRED)

    target_compile_options(scl PUBLIC "-O0")
    target_compile_options(scl PUBLIC "-g")
    target_compile_options(scl PUBLIC "--coverage")
    target_compile_options(scl PUBLIC "-fno-inline")
    target_link_libraries(scl PUBLIC gcov)
  endif()
  
  enable_testing()
  add_subdirectory(test)
endif()

if(DOCS)
  add_subdirectory(doc)
endif()
