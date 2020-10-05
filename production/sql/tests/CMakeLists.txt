# ##############################################################################
# Copyright (c) Gaia Platform LLC. All rights reserved.
# ##############################################################################

project(tests)

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
      ${CMAKE_CURRENT_SOURCE_DIR}/expected.txt " ")

  # Add a test to execute our test script.
  # We use the test infrastructure to execute the script
  # because we can leverage its ability to start the Gaia server.
  add_gtest(test_fdw test_fdw.cpp "${GAIA_STORAGE_ENGINE_TEST_INCLUDES}" "gaia_common;gaia_se_client")
endif()
