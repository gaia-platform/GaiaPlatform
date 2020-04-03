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
# The named arguments are required:  TARGET, SOURCES, INCLUDES, LIBRARIES
# Two optional arguments are after this: 
# [DEPENDENCIES] - for add_dependencies used for generation of flatbuffer files.  Defaults to ""
# [HAS_MAIN] - "{TRUE, 1, ON, YES, Y} indicates the test provides its own main function.  Defaults to "" (FALSE).
function(add_gtest TARGET SOURCES INCLUDES LIBRARIES)
#  message(STATUS "TARGET = ${TARGET}")
#  message(STATUS "SOURCES = ${SOURCES}")
#  message(STATUS "INCLUDES = ${INCLUDES}")
#  message(STATUS "LIBRARIES = ${LIBRARIES}")
#  message(STATUS "ARGV0 = ${ARGV0}")
#  message(STATUS "ARGV1 = ${ARGV1}")
#  message(STATUS "ARGV2 = ${ARGV2}")
#  message(STATUS "ARGV3 = ${ARGV3}")
#  message(STATUS "ARGV4 = ${ARGV4}")
#  message(STATUS "ARGV5 = ${ARGV5}")

  add_executable(${TARGET} ${SOURCES})

  if (NOT ("${ARGV4}" STREQUAL ""))
    add_dependencies(${TARGET} ${ARGV4})
  endif()

  target_include_directories(${TARGET} PRIVATE ${INCLUDES} ${GOOGLE_TEST_INC})
  if("${ARGV5}")
    set(GTEST_LIB "gtest")
  else()
    set(GTEST_LIB "gtest_main")
  endif()
  target_link_libraries(${TARGET} PRIVATE ${LIBRARIES} ${GTEST_LIB})

  set_target_properties(${TARGET} PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
  gtest_discover_tests(${TARGET})
endfunction(add_gtest)

# Gaia specific flatc helpers for generating headers
function(gaia_compile_flatbuffers_schema_to_cpp_opt SRC_FBS OPT)
  if(FLATBUFFERS_BUILD_LEGACY)
    set(OPT ${OPT};--cpp-std c++0x)
  else()
    # --cpp-std is defined by flatc default settings.
  endif()
  message(STATUS "`${SRC_FBS}`: add generation of C++ code with '${OPT}'")
  get_filename_component(SRC_FBS_DIR ${SRC_FBS} PATH)
  string(REGEX REPLACE "\\.fbs$" "_generated.h" GEN_HEADER ${SRC_FBS})
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${GEN_HEADER}
    COMMAND "${CMAKE_BINARY_DIR}/flatbuffers/flatc"
            --cpp --gen-mutable --gen-object-api --reflect-names
            --cpp-ptr-type flatbuffers::unique_ptr # Used to test with C++98 STLs
            --cpp-str-type gaia::common::nullable_string_t
            --gaiacpp --gen-setters
            ${OPT}
            -I ${CMAKE_CURRENT_SOURCE_DIR}
            -o ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/${SRC_FBS}
    DEPENDS ${CMAKE_BINARY_DIR}/flatbuffers/flatc 
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${SRC_FBS}
    COMMENT "Run generation: '${GEN_HEADER}'"
    VERBATIM)
    register_generated_output(${GEN_HEADER})
endfunction()

function(gaia_compile_flatbuffers_schema_to_cpp SRC_FBS)
  gaia_compile_flatbuffers_schema_to_cpp_opt(${SRC_FBS} "--no-includes;--gen-compare")
endfunction()
