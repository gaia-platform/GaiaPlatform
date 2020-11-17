# ##############################################################################
# Copyright (c) Gaia Platform LLC. All rights reserved.
# ##############################################################################

project(tests)

# To run the FDW tests, you need to perform the following steps:
# (1) Run cmake with the -DEXECUTE_FDW_TESTS=ON argument.
# (2) Execute 'make install' under build/sql after running make,
# to install the FDW binaries.
# (3) Ensure that local authentication for user postgres
# is set to 'trust' in file /etc/postgresql/12/main/pg_hba.conf.
# (4) Stop and restart Postgres service after doing (2) or (3).
if(EXECUTE_FDW_TESTS)
  find_program(BASH_PROGRAM bash)
  find_program(PG_CLIENT psql)

  if(BASH_PROGRAM AND PG_CLIENT)
    # Generate the command line for the test script.
    # It needs the paths from the build environment.
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/fdw_test_command.txt
      ${BASH_PROGRAM} " " ${CMAKE_CURRENT_SOURCE_DIR}/test.sh " "
        ${GAIA_REPO}/data/internal/airport " "
        ${PG_CLIENT} " "
        ${CMAKE_CURRENT_SOURCE_DIR}/setup.sql " "
        ${CMAKE_CURRENT_SOURCE_DIR}/test.sql " "
        ${CMAKE_CURRENT_SOURCE_DIR}/expected.txt)

    set(AIRLINES_DDL "${PROJECT_SOURCE_DIR}/airlines.ddl")

    file(COPY "${AIRLINES_DDL}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

    # Add a test to execute our test script.
    # We use the test infrastructure to execute the script
    # because we can leverage its ability to start the Gaia server.
    add_gtest(test_fdw test_fdw.cpp
      "${GAIA_STORAGE_ENGINE_TEST_INCLUDES};${GAIA_CATALOG_PUBLIC_INCLUDES}"
      "gaia_se_client;gaia_catalog;gaia_db_catalog_test")
  endif()
endif()
