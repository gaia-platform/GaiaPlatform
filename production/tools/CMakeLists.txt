###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

project(tools)

if(BUILD_GAIA_SDK)
  # clang-tidy takes 1:10 minutes on gaiat, for this reason, it is disabled for non-Debug builds.
  if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    unset(CMAKE_CXX_CLANG_TIDY)
  endif()
  add_subdirectory(gaia_translate)

  if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_CLANG_TIDY clang-tidy)
  endif()
endif()

add_subdirectory(gaia_dump)
add_subdirectory(gaia_db_extract)

# This should be enabled only for testing and improving .clang-tidy and .clang-format
# we don't want to build it as part of the normal build.
option(BUILD_GAIA_STYLE_TESTS "Build clang-tidy/clang-format tests" OFF)

if(BUILD_GAIA_STYLE_TESTS)
  add_subdirectory(gaia_style)
endif()

# The ping_pong test requires the gaia_db_server to be explicitly run.
# Keeping it disable, otherwise TeamCity fails.
# TODO, work on a better solution.
option(BUILD_PING_PONG_TEST "Build ping_pong test. Requires gaia_db_server to be running." OFF)

if(BUILD_PING_PONG_TEST)
  add_subdirectory(tests/ping_pong)
endif()
