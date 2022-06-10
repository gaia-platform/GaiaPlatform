###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

cmake_minimum_required(VERSION 3.16)

project(ping_pong)

set(CMAKE_CXX_STANDARD 17)

set(PING_PONG_DDL ${PROJECT_SOURCE_DIR}/ping_pong.ddl)
set(PING_PONG_RULESET ${PROJECT_SOURCE_DIR}/ping_pong.ruleset)

option(BUILD_SDK "Build ping_pong against the Gaia SDK instead of production" OFF)

if(BUILD_SDK)
  include(CMakeLists-sdk.cmake)
else()
  include(CMakeLists-internal.cmake)
endif()
