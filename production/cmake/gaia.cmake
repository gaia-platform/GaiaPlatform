#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Helper function to return the absolute path of the 
# repo root directory.  We use this to build absolute 
# include paths to code stored in the third-party 
# directory.  Note that this code assumes that the
# function is invoked from a directoy directly below
# the repo root (i.e. production or demos).
function(get_repo_root project_source_dir repo_dir)
  string(FIND ${project_source_dir} "/" repo_root_path REVERSE)
  string(SUBSTRING ${project_source_dir} 0 ${repo_root_path} project_repo)
  set(${repo_dir} "${project_repo}" PARENT_SCOPE)
endfunction()

set(TEST_SUCCESS "All tests passed!")

# Helper function for setting up our tests.
function(set_test target arg result)
  add_test(NAME ${target}_${arg} COMMAND ${target} ${arg})
  set_tests_properties(${target}_${arg} PROPERTIES PASS_REGULAR_EXPRESSION ${result})
endfunction(set_test)

# Helper function for setting up google tests.
# Assumes you want to link in the gtest provided
# main function
function(add_gtest TARGET SOURCES INCLUDES LIBRARIES)
  add_executable(${TARGET} ${SOURCES})
  target_include_directories(${TARGET} PRIVATE 
    ${INCLUDES}
    ${GOOGLE_TEST_INC}
  )
  target_link_libraries(${TARGET} PRIVATE 
    ${LIBRARIES}
    gtest_main
  )
  set_target_properties(${TARGET} PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
  gtest_discover_tests(${TARGET})
endfunction(add_gtest)
