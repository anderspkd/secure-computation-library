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

if (NOT SCL_DOXYGEN_BIN)
  find_program(doxygen doxygen REQUIRED)
else()
  if (EXISTS ${SCL_DOXYGEN_BIN} AND (NOT IS_DIRECTORY ${SCL_DOXYGEN_BIN}))
    set(doxygen ${SCL_DOXYGEN_BIN})
  else()
    message(FATAL_ERROR "\"${SCL_DOXYGEN_BIN}\" not a valid argument for doxygen binary.")
  endif()
endif()

add_custom_target(
  documentation
  COMMAND ${doxygen} "${CMAKE_SOURCE_DIR}/doc/DoxyConf"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Generate documentation for SCL"
  VERBATIM)
