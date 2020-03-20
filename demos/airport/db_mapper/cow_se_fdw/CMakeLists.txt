#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.10)

# Set the project name.

project(cow_se_fdw)

# Get version.

file(READ "version.config" PGCOW_SE_VERSION)
string(STRIP ${PGCOW_SE_VERSION} PGCOW_SE_VERSION)
set (PGCOW_SE_VERSION "${PGCOW_SE_VERSION}")
string(REGEX REPLACE "\\.[0-9]*$" "" PGCOW_SE_VERSION_MAJOR "${PGCOW_SE_VERSION}")
set (PGCOW_SE_VERSION_MAJOR "${PGCOW_SE_VERSION_MAJOR}")

# Get Postgres config settings from pg_config.

find_program(PG_CONFIG pg_config)
if(NOT PG_CONFIG)
  message(FATAL ERROR "Unable to find 'pg_config'")
endif(NOT PG_CONFIG)

exec_program(${PG_CONFIG} ARGS --version OUTPUT_VARIABLE PGCONFIG_OUTPUT)

string(REGEX REPLACE
    "^PostgreSQL[\t ]+([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\1.\\2.\\3"
    PGSQL_VERSION "${PGCONFIG_OUTPUT}")

unset(PGCONFIG_OUTPUT)

if(PGSQL_VERSION)
    string(REGEX REPLACE
        "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\1"
        PGSQL_VERSION_MAJOR "${PGSQL_VERSION}")
    string(REGEX REPLACE
        "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\2"
        PGSQL_VERSION_MINOR "${PGSQL_VERSION}")

    set(PGSQL_NUMERIC_VERSION ${PGSQL_VERSION_MAJOR}${PGSQL_VERSION_MINOR})
endif()

exec_program(${PG_CONFIG} ARGS --includedir OUTPUT_VARIABLE PGSQL_INCLUDEDIR)
exec_program(${PG_CONFIG} ARGS --includedir-server
  OUTPUT_VARIABLE PGSQL_INCLUDEDIR_SERVER)

exec_program(${PG_CONFIG} ARGS --pkglibdir OUTPUT_VARIABLE PGSQL_PKGLIBDIR)
exec_program(${PG_CONFIG} ARGS --sharedir OUTPUT_VARIABLE PGSQL_SHAREDIR)
exec_program(${PG_CONFIG} ARGS --bindir OUTPUT_VARIABLE PGSQL_BINDIR)
exec_program(${PG_CONFIG} ARGS --cppflags OUTPUT_VARIABLE PGSQL_CPPFLAGS)
exec_program(${PG_CONFIG} ARGS --ldflags OUTPUT_VARIABLE PGSQL_LDFLAGS)
exec_program(${PG_CONFIG} ARGS --libs OUTPUT_VARIABLE PGSQL_LIBS)

# Use default install location if not specified.

if (NOT DEFINED LIB_INSTALL_DIR)
  set (LIB_INSTALL_DIR lib)
endif()

# Build sources.

add_subdirectory (src)