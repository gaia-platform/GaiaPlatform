#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(test_schemas)

add_subdirectory(addr_book)
add_subdirectory(airport)
if (BUILD_GAIA_RELEASE)
add_subdirectory(incubator)
endif()
