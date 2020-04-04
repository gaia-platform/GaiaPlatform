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
function(add_gtest TARGET SOURCES INCLUDES LIBRARIES DEPENDENCIES)
  _add_gtest("${TARGET}" "${SOURCES}" "${INCLUDES}" "${LIBRARIES}" "${DEPENDENCIES}" "gtest_main")
endfunction(add_gtest)

function(add_gtest_no_main TARGET SOURCES INCLUDES LIBRARIES DEPENDENCIES)
  _add_gtest("${TARGET}" "${SOURCES}" "${INCLUDES}" "${LIBRARIES}" "${DEPENDENCIES}" "gtest")
endfunction(add_gtest_no_main)

function(_add_gtest TARGET SOURCES INCLUDES LIBRARIES DEPENDENCIES GTEST_LIB)
#  message(STATUS "TARGET = ${TARGET}")
#  message(STATUS "SOURCES = ${SOURCES}")
#  message(STATUS "INCLUDES = ${INCLUDES}")
#  message(STATUS "LIBRARIES = ${LIBRARIES}")
#  message(STATUS "DEPENDENCIES = ${DEPENDENCIES}")
#  message(STATUS "GTEST_LIB = ${GTEST_LIB}")

  add_executable(${TARGET} ${SOURCES})
  if (NOT ("${DEPENDENCIES}" STREQUAL ""))
    add_dependencies(${TARGET} ${DEPENDENCIES})
  endif()  
  target_include_directories(${TARGET} PRIVATE ${INCLUDES} ${GOOGLE_TEST_INC})
  target_link_libraries(${TARGET} PRIVATE ${LIBRARIES} ${GTEST_LIB})
  set_target_properties(${TARGET} PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
  gtest_discover_tests(${TARGET})
endfunction(_add_gtest)

# Gaia specific flatc helpers for generating headers
function(gaia_compile_flatbuffers_schema_to_cpp_opt SRC_FBS OPT)
  if(FLATBUFFERS_BUILD_LEGACY)
    set(OPT ${OPT};--cpp-std c++0x)
  else()
    # --cpp-std is defined by flatc default settings.
  endif()
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
