#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)
enable_testing()

# Set the project name.
project(catalog)

set(GAIA_PARSER_GENERATED_OUTPUTS "${CMAKE_CURRENT_BINARY_DIR}/parser/generated")
file(MAKE_DIRECTORY ${GAIA_PARSER_GENERATED_OUTPUTS})

add_subdirectory(catalog_manager)
add_subdirectory(gaiac)
add_subdirectory(parser)
