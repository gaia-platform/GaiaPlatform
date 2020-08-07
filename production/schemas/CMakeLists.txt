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

#
# System DDL files we are pulling in.
#
set(CATALOG_DDL ${GAIA_REPO}/production/catalog/catalog_manager/src/catalog.ddl)
set(EVENT_LOG_DDL ${GAIA_REPO}/production/rules/event_manager/schema/event_log.ddl)

add_subdirectory(system)
add_subdirectory(test)
