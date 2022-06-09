###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

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
    # Setup the DDL files for each test.
    file(COPY ${PROJECT_SOURCE_DIR}/array_schema.ddl DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY ${PROJECT_SOURCE_DIR}/airport_schema.ddl DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

    # Generate the command line for the test scripts.
    # They need the paths from the build environment.
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/fdw_test_array_command.txt
      ${BASH_PROGRAM} " " ${CMAKE_CURRENT_SOURCE_DIR}/test_fdw.sh " "
        ${PG_CLIENT} " "
        ${CMAKE_CURRENT_SOURCE_DIR}/array_setup.sql " "
        ${CMAKE_CURRENT_SOURCE_DIR}/array_test.sql " "
        ${CMAKE_CURRENT_SOURCE_DIR}/array_expected.txt)

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/fdw_test_airport_command.txt
      ${BASH_PROGRAM} " " ${CMAKE_CURRENT_SOURCE_DIR}/test_fdw.sh " "
        ${PG_CLIENT} " "
        ${CMAKE_CURRENT_SOURCE_DIR}/airport_setup.sql " "
        ${CMAKE_CURRENT_SOURCE_DIR}/airport_test.sql " "
        ${CMAKE_CURRENT_SOURCE_DIR}/airport_expected.txt " "
        ${CMAKE_CURRENT_SOURCE_DIR}/airport)

    # Add a test to execute our test scripts.
    # We use the test infrastructure to execute the script
    # because we can leverage its ability to start the Gaia server.
    add_gtest(test_fdw test_fdw.cpp
      "${GAIA_DB_CORE_TEST_INCLUDES};${GAIA_CATALOG_PUBLIC_INCLUDES}"
      "gaia_db_client;gaia_catalog;gaia_db_catalog_test")
  endif()
endif()
