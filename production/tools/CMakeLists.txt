#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(tools)

add_subdirectory(gaia_generate)

if (BUILD_GAIA_RELEASE)
  add_subdirectory(gaia_translate)
endif()

add_subdirectory(gaia_dump)

# This should be enabled only for testing and improving .clang-tidy and .clang-format
# we don't want to build it as part of the normal build.
option(BUILD_GAIA_STYLE_TESTS "Build clang-tidy/clang-format tests" OFF)

if(BUILD_GAIA_STYLE_TESTS)
  add_subdirectory(gaia_style)
endif()

# The ping_pong test requires the gaia_db_server to be explicitly run. Keeping it disable otherwise
# TeamCity fails. TODO work on a better solution.
option(BUILD_PING_PONG_TEST "Build ping_pong test. Requires gaia_db_server to be running." OFF)

if(BUILD_PING_PONG_TEST)
  add_subdirectory(tests/ping_pong)
endif()
