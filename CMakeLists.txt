# SCL --- Secure Computation Library
# Copyright (C) 2022 Anders Dalskov
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

cmake_minimum_required( VERSION 3.14 )

project( scl VERSION 5.1.1 DESCRIPTION "Secure Computation Library" )

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(WITH_EC "Include support for elliptic curves (requires GMP)" ON)

message(STATUS "CMAKE_BUILD_TYPE=" ${CMAKE_BUILD_TYPE})
message(STATUS "WITH_EC=" ${WITH_EC})

if(WITH_EC MATCHES ON)
  find_library(GMP gmp libgmp REQUIRED)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -march=native -Wall -Wextra -pedantic -Werror -std=gnu++17")

set(SCL_SOURCE_FILES
  src/scl/util/str.cc
  src/scl/util/prg.cc
  src/scl/util/sha3.cc
  src/scl/util/sha256.cc

  src/scl/math/mersenne61.cc
  src/scl/math/mersenne127.cc

  src/scl/net/config.cc
  src/scl/net/mem_channel.cc
  src/scl/net/threaded_sender.cc
  src/scl/net/network.cc

  src/scl/simulation/simulator.cc
  src/scl/simulation/simulate_recv_time.cc
  src/scl/simulation/config.cc
  src/scl/simulation/event.cc
  src/scl/simulation/measurement.cc
  src/scl/simulation/result.cc
  src/scl/simulation/channel.cc
  src/scl/simulation/context.cc)

if(WITH_EC MATCHES ON)
  set(SCL_SOURCE_FILES ${SCL_SOURCE_FILES}
    src/scl/math/ops_gmp_ff.cc
    src/scl/math/secp256k1_field.cc
    src/scl/math/secp256k1_curve.cc
    src/scl/math/secp256k1_order.cc
    src/scl/math/number.cc)
endif()

set(SCL_HEADERS "${CMAKE_SOURCE_DIR}/include")

include_directories(${SCL_HEADERS})

if(CMAKE_BUILD_TYPE MATCHES "Release")
  set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -O2")
  add_library(scl SHARED ${SCL_SOURCE_FILES})

  set_target_properties(scl PROPERTIES VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

  message(STATUS "CMAKE_INSTALL_PREFIX=" ${CMAKE_INSTALL_PREFIX})
  
  install(TARGETS scl
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib)

  install(DIRECTORY "${SCL_HEADERS}/"
    DESTINATION include
    FILES_MATCHING PATTERN "*.h")

endif()

if(CMAKE_BUILD_TYPE MATCHES "Debug")

  enable_testing()

  set(SCL_TEST_SOURCE_FILES
    test/scl/main.cc

    test/scl/util/test_prg.cc
    test/scl/util/test_sha3.cc
    test/scl/util/test_sha256.cc
    test/scl/util/test_traits.cc

    test/scl/gf7.cc
    test/scl/math/test_mersenne61.cc
    test/scl/math/test_mersenne127.cc
    test/scl/math/test_vec.cc
    test/scl/math/test_mat.cc
    test/scl/math/test_la.cc
    test/scl/math/test_ff.cc
    test/scl/math/test_z2k.cc
    test/scl/math/test_poly.cc

    test/scl/ss/test_additive.cc
    test/scl/ss/test_shamir.cc
    test/scl/ss/test_feldman.cc

    test/scl/net/util.cc
    test/scl/net/test_config.cc
    test/scl/net/test_mem_channel.cc
    test/scl/net/test_tcp.cc
    test/scl/net/test_tcp_channel.cc
    test/scl/net/test_threaded_sender.cc
    test/scl/net/test_network.cc
    test/scl/net/test_shared_deque.cc
    test/scl/net/test_channel.cc

    test/scl/protocol/test_protocol.cc

    test/scl/simulation/test_simulator.cc
    test/scl/simulation/test_config.cc
    test/scl/simulation/test_event.cc
    test/scl/simulation/test_context.cc
    test/scl/simulation/test_result.cc
    test/scl/simulation/test_measurement.cc
    test/scl/simulation/test_mem_channel_buffer.cc
    test/scl/simulation/test_env.cc)

  if(WITH_EC MATCHES ON)
    set(SCL_TEST_SOURCE_FILES ${SCL_TEST_SOURCE_FILES}
      test/scl/math/test_secp256k1.cc
      test/scl/math/test_number.cc)
    add_compile_definitions(SCL_ENABLE_EC_TESTS)
  endif()

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -fsanitize=address")
  find_package(Catch2 REQUIRED)
  include(CTest)
  include(Catch)
  include(${CMAKE_SOURCE_DIR}/cmake/CodeCoverage.cmake)

  # Lower the max size of Vec/Mat reads to speed up tests
  add_compile_definitions(MAX_VEC_READ_SIZE=1024)
  add_compile_definitions(MAX_MAT_READ_SIZE=1024)
  add_compile_definitions(SCL_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/test/data/")

  add_executable(scl_test ${SCL_SOURCE_FILES} ${SCL_TEST_SOURCE_FILES})
  target_link_libraries(scl_test Catch2::Catch2 pthread)

  if(WITH_EC MATCHES ON)
    target_link_libraries(scl_test ${GMP})
  endif()

  catch_discover_tests(scl_test)

  append_coverage_compiler_flags()

  # Tell lcov to ignore system STL headers in order to make the coverage
  # output more precise.
  setup_target_for_coverage_lcov(
    NAME coverage
    EXECUTABLE scl_test
    EXCLUDE "/usr/include/*" "test/*" "/usr/lib/*" "/usr/local/*")

endif()

message(STATUS "CXX_FLAGS=" ${CMAKE_CXX_FLAGS})
