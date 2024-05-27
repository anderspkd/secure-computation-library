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

set(SCL_SOURCE_FILES_TEST
  scl/util/test_prg.cc
  scl/util/test_sha3.cc
  scl/util/test_sha256.cc
  scl/util/test_ecdsa.cc
  scl/util/test_cmdline.cc
  scl/util/test_merkle.cc
  scl/util/test_bitmap.cc
  scl/util/test_measurement.cc

  scl/serialization/test_serializer.cc

  scl/gf7.cc
  scl/math/test_mersenne61.cc
  scl/math/test_mersenne127.cc
  scl/math/test_vector.cc
  scl/math/test_matrix.cc
  scl/math/test_la.cc
  scl/math/test_ff.cc
  scl/math/test_z2k.cc
  scl/math/test_poly.cc
  scl/math/test_array.cc

  scl/math/test_secp256k1.cc
  scl/math/test_number.cc

  scl/ss/test_additive.cc
  scl/ss/test_shamir.cc
  scl/ss/test_feldman.cc
  scl/ss/test_pedersen.cc

  scl/coro/test_task.cc
  scl/coro/test_batch.cc

  scl/net/util.cc
  scl/net/test_config.cc
  scl/net/test_loopback.cc
  scl/net/test_network.cc
  scl/net/test_packet.cc

  scl/protocol/test_protocol.cc

  scl/simulation/test_event.cc
  scl/simulation/test_context.cc
  scl/simulation/test_config.cc
  scl/simulation/test_channel.cc
  scl/simulation/test_simulator.cc
)

Include(FetchContent)

# get Catch2 and compile it. We cannot use the system provided
# version, since that is compiled with C++14 which causes linker
# errors...
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(Catch2)

include(CTest)
include(Catch)

add_compile_definitions(SCL_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/test/data/")

add_executable(scl_test ${SCL_SOURCE_FILES_TEST})
target_link_libraries(scl_test
  PRIVATE Catch2::Catch2WithMain
  PRIVATE scl
  PRIVATE pthread
  PRIVATE gmp)
catch_discover_tests(scl_test)

if(SCL_BUILD_TEST_WITH_COVERAGE)

  if (NOT SCL_GCOV_BIN)
    set(SCL_GCOV_BIN "gcov")
  endif()

  ## stuff that SCL uses, but which we are not interested in
  ## generating coverage of.
  set(SCL_COVERAGE_EXCLUDES "")
  list(APPEND SCL_COVERAGE_EXCLUDES "'/usr/*'")
  list(APPEND SCL_COVERAGE_EXCLUDES "'${CMAKE_BINARY_DIR}/*'")
  list(APPEND SCL_COVERAGE_EXCLUDES "'${CMAKE_SOURCE_DIR}/test/*'")

  add_custom_target(coverage
    COMMAND lcov --ignore-errors mismatch --gcov-tool ${SCL_GCOV_BIN} -d ${CMAKE_BINARY_DIR} -b ${CMAKE_BINARY_DIR} -z
    COMMAND lcov --ignore-errors mismatch --gcov-tool ${SCL_GCOV_BIN} -d ${CMAKE_BINARY_DIR} -b ${CMAKE_BINARY_DIR} -c -i -o cov.base
    COMMAND scl_test
    COMMAND lcov --ignore-errors mismatch --gcov-tool ${SCL_GCOV_BIN} -d ${CMAKE_BINARY_DIR} -b ${CMAKE_BINARY_DIR} -c -o cov.cap
    COMMAND lcov --ignore-errors mismatch --gcov-tool ${SCL_GCOV_BIN} -a cov.base -a cov.cap -o cov.total
    COMMAND wc -l cov.total
    COMMAND lcov --ignore-errors mismatch --gcov-tool ${SCL_GCOV_BIN} --remove cov.total ${SCL_COVERAGE_EXCLUDES} -o cov.info
    COMMAND genhtml --demangle-cpp --ignore-errors mismatch -o coverage cov.info

    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    BYPRODUCTS cov.base cov.cap cov.total cov.info)

endif()
