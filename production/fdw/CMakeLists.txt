###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

project(fdw)

# Use existence of pg_config as a proxy for Postgres being installed.
find_program(PG_CONFIG pg_config)
if(NOT PG_CONFIG)
  message(WARNING "Postgres server does not appear to be installed, skipping build of ${CMAKE_CURRENT_SOURCE_DIR}.")
  return()
endif(NOT PG_CONFIG)

# Use production major.minor as version of the FDW plugin. The patch is discarded because other plugins do not use it.
# Using the patch would cause FDW files to keep getting added to PG because installing a new version
# doesn't remove the old one, unless it overwrites it.
set(PG_GAIA_FDW_VERSION "${production_VERSION_MAJOR}.${production_VERSION_MINOR}")

# Get Postgres config settings from pg_config.
exec_program(
  ${PG_CONFIG} ARGS
  --includedir OUTPUT_VARIABLE
  PGSQL_INCLUDEDIR)
exec_program(
  ${PG_CONFIG} ARGS
  --includedir-server OUTPUT_VARIABLE
  PGSQL_INCLUDEDIR_SERVER)
exec_program(
  ${PG_CONFIG} ARGS
  --pkglibdir OUTPUT_VARIABLE
  PGSQL_PKGLIBDIR)
exec_program(
  ${PG_CONFIG} ARGS
  --sharedir OUTPUT_VARIABLE
  PGSQL_SHAREDIR)
exec_program(
  ${PG_CONFIG} ARGS
  --bindir OUTPUT_VARIABLE
  PGSQL_BINDIR)
exec_program(
  ${PG_CONFIG} ARGS
  --cppflags OUTPUT_VARIABLE
  PGSQL_CPPFLAGS)
exec_program(
  ${PG_CONFIG} ARGS
  --ldflags OUTPUT_VARIABLE
  PGSQL_LDFLAGS)
exec_program(
  ${PG_CONFIG} ARGS
  --libs OUTPUT_VARIABLE
  PGSQL_LIBS)

# Use default install location if not specified.
if(NOT DEFINED LIB_INSTALL_DIR)
  set(LIB_INSTALL_DIR lib)
endif()

# Build sources.
add_subdirectory(src)

# Add tests.
add_subdirectory(tests)
