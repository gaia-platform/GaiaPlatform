# - Returns a version string from Git tags
#
# This function inspects the annotated git tags for the project and returns a string
# into a CMake variable
#
#  get_git_version(<var>)
#
# - Example
#
# include(GetGitVersion)
# get_git_version(GIT_VERSION)
#
# Requires CMake 2.8.11+
find_package(Git)

if(__get_git_version)
  return()
endif()
set(__get_git_version INCLUDED)

function(get_git_version var)
  set(GIT_VERSION "v0.0.0")

  message(STATUS "git Version: ${GIT_VERSION}")
  set(${var} ${GIT_VERSION} PARENT_SCOPE)
endfunction()
