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

project( scl_examples VERSION 1.0 DESCRIPTION "SCL example programs" )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -O2 -std=gnu++17" )

find_library(GMP gmp libgmp REQUIRED)
find_library(SCL scl libscl REQUIRED)

## primitives
set( primitives "01_primitives" )
add_executable( "${primitives}" "${primitives}.cc" )
target_link_libraries( "${primitives}" "${SCL}" "${GMP}" pthread )

## finite fields
set( finite_fields "02_finite_fields" )
add_executable( "${finite_fields}" "${finite_fields}.cc" )
target_link_libraries( "${finite_fields}" "${SCL}" "${GMP}" pthread )

## secret sharing
set( secret_sharing "03_secret_sharing" )
add_executable( "${secret_sharing}" "${secret_sharing}.cc" )
target_link_libraries( "${secret_sharing}" "${SCL}" "${GMP}" pthread )

## simple networking
set( networking "04_networking" )
add_executable( "${networking}" "${networking}.cc" )
target_link_libraries( "${networking}" "${SCL}" "${GMP}" pthread )
