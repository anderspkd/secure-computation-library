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

project( scl VERSION 1.0.0 DESCRIPTION "Secure Computation Library" )

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "CMAKE_BUILD_TYPE=" ${CMAKE_BUILD_TYPE})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -march=native -Wall -Wextra -pedantic -Werror -std=gnu++17")

set(SCL_SOURCE_FILES
  src/scl/prg.cc
  src/scl/hash.cc

  src/scl/math/str.cc
  src/scl/math/mersenne61.cc
  src/scl/math/mersenne127.cc

  src/scl/net/config.cc
  src/scl/net/mem_channel.cc
  src/scl/net/tcp_channel.cc
  src/scl/net/threaded_sender.cc
  src/scl/net/tcp_utils.cc
  src/scl/net/network.cc
  src/scl/net/discovery/server.cc
  src/scl/net/discovery/client.cc)

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

  set(SCL_TEST_SOURCE_FILES
    test/scl/main.cc

    test/scl/test_hash.cc
    test/scl/test_prg.cc

    test/scl/math/test_mersenne61.cc
    test/scl/math/test_mersenne127.cc
    test/scl/math/test_vec.cc
    test/scl/math/test_mat.cc
    test/scl/math/test_la.cc
    test/scl/math/test_ff.cc
    test/scl/math/test_z2k.cc

    test/scl/ss/test_additive.cc
    test/scl/ss/test_poly.cc
    test/scl/ss/test_shamir.cc

    test/scl/net/util.cc
    test/scl/net/test_config.cc
    test/scl/net/test_mem_channel.cc
    test/scl/net/test_tcp_channel.cc
    test/scl/net/test_threaded_sender.cc
    test/scl/net/test_network.cc
    test/scl/net/test_discover.cc

    test/scl/p/test_simple.cc)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
    find_package(Catch2 REQUIRED)
    include(CTest)
    include(Catch)
    include(${CMAKE_SOURCE_DIR}/cmake/CodeCoverage.cmake)

    # Tests that check bounds for reading/writing are sped up considerably by
    # lowering said bounds.
    add_compile_definitions(MAX_VEC_READ_SIZE=1024)
    add_compile_definitions(MAX_MAT_READ_SIZE=1024)
    add_compile_definitions(SCL_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/test/data/")

    add_executable(scl_test ${SCL_SOURCE_FILES} ${SCL_TEST_SOURCE_FILES})
    target_link_libraries(scl_test Catch2::Catch2 pthread)
    catch_discover_tests(scl_test)

    append_coverage_compiler_flags()

    # Tell lcov to ignore system STL headers in order to make the coverage
    # output more precise.
    setup_target_for_coverage_lcov(
      NAME coverage
      EXECUTABLE scl_test
      EXCLUDE "/usr/include/*" "test/*" "/usr/lib/*" "/usr/local/*")

endif()
