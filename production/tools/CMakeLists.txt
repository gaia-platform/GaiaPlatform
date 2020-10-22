#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(tools)

add_subdirectory(gaia_generate)
add_subdirectory(gaia_dump)

# This should be enabled only for testing and improving .clang-tidy and .clang-format
# we don't want to build it as part of the normal build.
option(BUILD_GAIA_STYLE_TESTS "Build clang-tidy/clang-format tests" OFF)

if(BUILD_GAIA_STYLE_TESTS)
  add_subdirectory(gaia_style)
endif()
