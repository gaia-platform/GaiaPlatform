#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Helper function to return the absolute path of the 
# repo root directory of the given repo name.  We 
# use this to build absolute include paths to
# code stored in the third-party directory.  
function(get_repo_root project_source_dir repo_name repo_dir)
  string(FIND ${project_source_dir} ${repo_name} repo_path_start REVERSE)
  string(LENGTH ${repo_name} repo_name_len)
  math(EXPR repo_path_len "${repo_path_start} + ${repo_name_len}")
  string(SUBSTRING ${project_source_dir} 0 ${repo_path_len} project_repo)
  set(${repo_dir} "${project_repo}" PARENT_SCOPE)
endfunction()

# Helper function for setting up our tests.
function(set_test target arg result)
  add_test(NAME ${target}_${arg} COMMAND ${target} ${arg})
  set_tests_properties(${target}_${arg} PROPERTIES PASS_REGULAR_EXPRESSION ${result})
endfunction(set_test)
