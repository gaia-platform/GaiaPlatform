#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.12)
enable_testing()

# Set the project name.
project(catalog)

add_subdirectory(catalog_manager)
add_subdirectory(gaiac)
add_subdirectory(parser)
