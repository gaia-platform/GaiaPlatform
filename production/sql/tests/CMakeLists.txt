# ##############################################################################
# Copyright (c) Gaia Platform LLC. All rights reserved.
# ##############################################################################

project(tests)

find_program(BASH_PROGRAM bash)
find_program(PG_CLIENT psql)
find_program(PG_ISREADY pg_isready)

if(BASH_PROGRAM
   AND PG_CLIENT
   AND PG_ISREADY)
  # try to connect to postgres server
  execute_process(COMMAND "${PG_ISREADY}" RESULT_VARIABLE pg_up)
  if(pg_up EQUAL 0)
    add_test(
      NAME test_sql_queries
      COMMAND
        ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/test.sh
        ${GAIA_REPO}/data/internal/airport ${PG_CLIENT}
        ${CMAKE_CURRENT_SOURCE_DIR}/setup.sql
        ${CMAKE_CURRENT_SOURCE_DIR}/test.sql
        ${CMAKE_CURRENT_SOURCE_DIR}/expected.txt)
  else()
    message(WARNING "Postgres server does not appear to be running.")
  endif(pg_up EQUAL 0)
endif(
  BASH_PROGRAM
  AND PG_CLIENT
  AND PG_ISREADY)
