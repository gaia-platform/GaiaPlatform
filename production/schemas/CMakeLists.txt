#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.12)

project(schemas)

set(GAIA_SCHEMAS_INC "${PROJECT_SOURCE_DIR}")

#
# Catalog tool we use to generate headers.
#
set(GAIA_GENERATE ${GAIA_PROD_BUILD}/catalog/gaiac/gaiac)

file(MAKE_DIRECTORY ${GAIA_GENERATED_SCHEMAS})

add_subdirectory(system)
add_subdirectory(test)
