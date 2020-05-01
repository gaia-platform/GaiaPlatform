#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

project(tests)

find_program (BASH_PROGRAM bash)
find_program (PG_CLIENT psql)

if (BASH_PROGRAM AND PG_CLIENT)
  add_test (
    NAME test_sql_queries
    COMMAND ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/test.sh
        ${GAIA_REPO}/data/internal/airport
        ${PG_CLIENT}
        ${CMAKE_CURRENT_SOURCE_DIR}/setup.sql
        ${CMAKE_CURRENT_SOURCE_DIR}/test.sql
        ${CMAKE_CURRENT_SOURCE_DIR}/expected.txt
    )
endif (BASH_PROGRAM)
