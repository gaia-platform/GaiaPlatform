#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(tools)

add_subdirectory(gaia_generate)

# This should be enabled only for testing and improving .clang-tidy and .clang-format
# we don't want to build it as part of the normal build.
add_subdirectory(gaia_style)
