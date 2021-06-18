#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Helper function to return the absolute path of the
# repo root directory.  We use this to build absolute
# include paths to code stored in the third-party
# directory.  Note that this code assumes that the
# function is invoked from a directory directly below
# the repo root (i.e. production or demos).
function(get_repo_root project_source_dir repo_dir)
  string(FIND ${project_source_dir} "/" repo_root_path REVERSE)
  string(SUBSTRING ${project_source_dir} 0 ${repo_root_path} project_repo)
  set(${repo_dir} "${project_repo}" PARENT_SCOPE)
endfunction()

#
# Helper function for setting up our tests.
#
function(set_test target arg result)
  add_test(NAME ${target}_${arg} COMMAND ${target} ${arg})
  set_tests_properties(${target}_${arg} PROPERTIES PASS_REGULAR_EXPRESSION ${result})
endfunction(set_test)

#
# Helper function for setting up google tests.
# The named arguments are required:  TARGET, SOURCES, INCLUDES, LIBRARIES
# Three optional arguments are after this:
# [DEPENDENCIES] - for add_dependencies used for generation of flatbuffer files.  Defaults to "".
# [HAS_MAIN] - "{TRUE, 1, ON, YES, Y} indicates the test provides its own main function.  Defaults to "" (FALSE).
# [ENV] - a semicolon-delimited list of key-value pairs for environment variables to be passed to the test. Defaults to "".
#
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
    set(GTEST_LIB "gtest;gtest_main")
  endif()
  target_link_libraries(${TARGET} PRIVATE ${LIBRARIES} ${GTEST_LIB})

  if(ENABLE_STACKTRACE)
    target_link_libraries(${TARGET} PRIVATE gaia_stack_trace)
  endif()

  if(NOT ("${ARGV6}" STREQUAL ""))
    set(ENV "${ARGV6}")
  else()
    set(ENV "")
  endif()

  target_link_libraries(${TARGET} PUBLIC gaia_build_options)
  set_target_properties(${TARGET} PROPERTIES CXX_CLANG_TIDY "")
  gtest_discover_tests(${TARGET} PROPERTIES ENVIRONMENT "${ENV}")
endfunction(add_gtest)

#
# Gaia specific flatc helpers for generating headers
#
function(gaia_register_generated_output file_name)
  get_property(tmp GLOBAL PROPERTY FBS_GENERATED_OUTPUTS)
  list(APPEND tmp ${file_name})
  set_property(GLOBAL PROPERTY FBS_GENERATED_OUTPUTS ${tmp})
endfunction(gaia_register_generated_output)

function(gaia_get_generated_output generated_files)
  get_property(tmp GLOBAL PROPERTY FBS_GENERATED_OUTPUTS)
  set(${generated_files} ${tmp} PARENT_SCOPE)
endfunction(gaia_get_generated_output)

function(gaia_compile_flatbuffers_schema_to_cpp_opt SRC_FBS OPT OUTPUT_DIR)
  if(FLATBUFFERS_BUILD_LEGACY)
    set(OPT ${OPT};--cpp-std c++0x)
  else()
    # --cpp-std is defined by flatc default settings.
  endif()
  get_filename_component(SRC_FBS_DIR ${SRC_FBS} PATH)
  get_filename_component(SRC_FBS_FILE ${SRC_FBS} NAME)
  string(REGEX REPLACE "\\.fbs$" "_generated.h" GEN_HEADER ${SRC_FBS_FILE})

  add_custom_command(
    OUTPUT ${OUTPUT_DIR}/${GEN_HEADER}
    COMMAND "${GAIA_PROD_BUILD}/flatbuffers/flatc"
    --cpp --gen-mutable --gen-object-api --reflect-names
    --cpp-ptr-type flatbuffers::unique_ptr # Used to test with C++98 STLs
    --cpp-str-type gaia::direct_access::nullable_string_t
    --cpp-str-flex-ctor
    --gaiacpp
    ${OPT}
    -I ${CMAKE_CURRENT_SOURCE_DIR}
    -o ${OUTPUT_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/${SRC_FBS}
    DEPENDS ${GAIA_PROD_BUILD}/flatbuffers/flatc
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${SRC_FBS}
    COMMENT "Run generation: '${GEN_HEADER}'"
    VERBATIM)
  gaia_register_generated_output(${OUTPUT_DIR}/${GEN_HEADER})
endfunction()

# Gaia specific flatc helpers for generating headers
# Optional parameter [OUTPUT_DIR], default is ${CMAKE_CURRENT_SOURCE_DIR}
function(gaia_compile_flatbuffers_schema_to_cpp SRC_FBS)
  # message(STATUS "ARGV1=${ARGV1}")
  if (NOT ("${ARGV1}" STREQUAL ""))
    set(OUTPUT_DIR "${ARGV1}")
  else()
    set(OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
  endif()
  message(STATUS "OUTPUT_DIR = ${OUTPUT_DIR}")

  gaia_compile_flatbuffers_schema_to_cpp_opt(${SRC_FBS} "--no-includes;--gen-compare" ${OUTPUT_DIR})
endfunction()

# Creates a CMake target that loads schema definition specified by DDL_FILE
# into the Gaia database. It then generates the EDC code to access the
# database programmatically, and build the EDC classes into a static library.
#
# The generated file is written to: ${OUTPUT_FOLDER}/gaia_${DDL_NAME}.h
# where DDL_NAME is DDL_FILE with no extension.
#
# Args:
# - DDL_FILE: [optional] the path to the .ddl file.
#     If not specified, the function generates the EDC code for the database
#     specified by DATABASE_NAME.
# - OUTPUT_DIR: [optional] directory where the header files will be generated.
#     If not specified the default value is ${GAIA_GENERATED_CODE}/${DATABASE_NAME}
# - LIB_NAME: [optional] the name of the generated target.
#     If not specified the default value is edc_${DDL_NAME}.
# - DATABASE_NAME: [optional] name of the database the headers are generated from.
#     If not specified the database name will be inferred as ${DDL_NAME}.
#     This is a temporary workaround, until we improve gaiac.
function(process_schema_internal)
  set(options "")
  set(oneValueArgs DDL_FILE OUTPUT_DIR LIB_NAME DATABASE_NAME INSTANCE_NAME)
  set(multiValueArgs "")
  cmake_parse_arguments("ARG" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT DEFINED ARG_DDL_FILE AND NOT DEFINED ARG_DATABASE_NAME)
    message(FATAL_ERROR "You must specify either the DDL_FILE or the DATABASE_NAME!")
  endif()

  # If the database name is not specified we infer it from the DDL file name.
  if(NOT DEFINED ARG_DATABASE_NAME)
    get_filename_component(DDL_NAME ${ARG_DDL_FILE} NAME)
    string(REPLACE ".ddl" "" DDL_NAME ${DDL_NAME})
    set(ARG_DATABASE_NAME ${DDL_NAME})
    message(VERBOSE "DATABASE_NAME not specified, using: ${ARG_DATABASE_NAME}.")
  endif()

  if(NOT DEFINED ARG_OUTPUT_DIR)
    set(ARG_OUTPUT_DIR ${GAIA_GENERATED_CODE}/${ARG_DATABASE_NAME})
    message(VERBOSE "OUTPUT_DIR not specified, defaulted to: ${ARG_OUTPUT_DIR}.")
  endif()

  set(EDC_HEADER_FILE ${ARG_OUTPUT_DIR}/gaia_${ARG_DATABASE_NAME}.h)
  set(EDC_CPP_FILE ${ARG_OUTPUT_DIR}/gaia_${ARG_DATABASE_NAME}.cpp)

  message(VERBOSE "Adding target to generate EDC code for database ${ARG_DATABASE_NAME}...")

  string(RANDOM GAIAC_INSTANCE_NAME)

  if (DEFINED ARG_DDL_FILE)
    add_custom_command(
      COMMENT "Generating EDC code for file ${ARG_DDL_FILE} and database ${ARG_DATABASE_NAME}..."
      OUTPUT ${EDC_HEADER_FILE}
      OUTPUT ${EDC_CPP_FILE}
      COMMAND ${GAIA_PROD_BUILD}/catalog/gaiac/gaiac
        -t ${GAIA_PROD_BUILD}/db/core
        -o ${ARG_OUTPUT_DIR}
        -g ${ARG_DDL_FILE}
        -d ${ARG_DATABASE_NAME}
        -n ${GAIAC_INSTANCE_NAME}
      DEPENDS ${ARG_DDL_FILE}
      DEPENDS gaiac
    )
  else()
    add_custom_command(
      COMMENT "Generating EDC code for database ${ARG_DATABASE_NAME}..."
      OUTPUT ${EDC_HEADER_FILE}
      OUTPUT ${EDC_CPP_FILE}
      COMMAND ${GAIA_PROD_BUILD}/catalog/gaiac/gaiac
        -t ${GAIA_PROD_BUILD}/db/core
        -o ${ARG_OUTPUT_DIR}
        -d ${ARG_DATABASE_NAME}
        -n ${GAIAC_INSTANCE_NAME}
        -g
      DEPENDS gaiac
    )
  endif()

  if(NOT DEFINED ARG_LIB_NAME)
    set(ARG_LIB_NAME "edc_${ARG_DATABASE_NAME}")
    message(VERBOSE "LIB_NAME not specified, using: ${ARG_LIB_NAME}.")
  endif()

  add_library(${ARG_LIB_NAME}
    ${EDC_CPP_FILE})

  target_link_libraries(${ARG_LIB_NAME} PUBLIC gaia_build_options)
  target_include_directories(${ARG_LIB_NAME} PUBLIC ${ARG_OUTPUT_DIR})
  target_include_directories(${ARG_LIB_NAME} PUBLIC ${FLATBUFFERS_INC})
  target_include_directories(${ARG_LIB_NAME} PRIVATE ${GAIA_INC})
  target_link_libraries(${ARG_LIB_NAME} PUBLIC gaia_direct)
endfunction()

# Stop CMake if the given parameter was not passed to the function.
macro(check_param PARAM)
  if(NOT DEFINED ${PARAM})
    message(FATAL_ERROR "The parameter ${PARAM} is required!")
  endif()
endmacro()

# Builds everything required for an end to end gtest that uses gaiac and gaiat.
# Currently starts and stops a default server instance.
#
# Inputs:
#
# TARGET_NAME - name of the gtest
# DDL_FILE - input gaia ddl file, but gtest will link to already build edc lib
# RULESET_FILE - input ruleset file
# EDC_LIBRARY - name of generated EDC library from DDL that this gtest will link to
# [PREVIOUS_TARGET_NAME] - for now these test use the same db instance so serialize build
#   by specifying a previous target.  This argument is optional
# TARGET_SOURCES - semicolon delimited list of gtest sources
# TARGET_INCLUDES - include list for gtest
# [TARGET_LIBRARIES] - other libraries to link to excluding the EDC_LIBRARY.  If not specified
#   the gtest will be linked to "rt;gaia_system;gaia_db_catalog_test;EDC_LIBRARY"
#
# Outputs:
#
# RULESET_FILE.CPP - translated ruleset source under ${GAIA_GENERATED_CODE}
# 
function(add_gaia_sdk_gtest)
  set(options "")
  set(oneValueArgs TARGET_NAME DDL_FILE RULESET_FILE DATABASE_NAME PREVIOUS_TARGET_NAME)
  set(multiValueArgs TARGET_SOURCES TARGET_INCLUDES TARGET_LIBRARIES)
  cmake_parse_arguments("ARG" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  check_param(ARG_TARGET_NAME)
  check_param(ARG_DDL_FILE)
  check_param(ARG_RULESET_FILE)
  check_param(ARG_DATABASE_NAME)
  check_param(ARG_TARGET_SOURCES)
  check_param(ARG_TARGET_INCLUDES)

  set(EDC_INCLUDE "${GAIA_GENERATED_CODE}/${ARG_DATABASE_NAME}")
  set(EDC_LIBRARY "edc_${ARG_DATABASE_NAME}")
  if (NOT DEFINED ARG_TARGET_LIBRARIES)
    set(ARG_TARGET_LIBRARIES "rt;gaia_system;gaia_db_catalog_test;${EDC_LIBRARY}")
  endif()

  get_filename_component(RULESET_NAME ${ARG_RULESET_FILE} NAME)
  string(REPLACE ".ruleset" "" RULESET_NAME ${RULESET_NAME})

  set(RULESET_CPP_NAME ${RULESET_NAME}_ruleset.cpp)
  set(RULESET_CPP_OUT ${GAIA_GENERATED_CODE}/${RULESET_CPP_NAME})

  set(GAIAC_CMD "${GAIA_PROD_BUILD}/catalog/gaiac/gaiac")
  set(GAIAT_CMD "${GAIA_PROD_BUILD}/tools/gaia_translate/gaiat")

  add_custom_command(
    COMMENT "Compiling ${RULESET_FILE}..."
    OUTPUT ${RULESET_CPP_OUT}
    COMMAND ${GAIA_PROD_BUILD}/db/core/gaia_db_server --disable-persistence &
    COMMAND sleep 1
    COMMAND ${GAIAC_CMD} ${ARG_DDL_FILE}
    COMMAND ${GAIAT_CMD} ${ARG_RULESET_FILE} -output ${RULESET_CPP_OUT} --
      -I ${GAIA_INC}
      -I ${FLATBUFFERS_INC}
      -I ${GAIA_SPDLOG_INC}
      -I ${EDC_INCLUDE}
      -I /usr/include/clang/10/include/
      -std=c++${CMAKE_CXX_STANDARD}
    COMMAND pkill -f -KILL gaia_db_server &

    # In some contexts, the next attempt to start gaia_db_server precedes this kill, leading
    # to a build failure. A short sleep is currently fixing that, but may not be the
    # correct long-term solution.
    COMMAND sleep 1
    DEPENDS ${GAIAC_CMD}
    DEPENDS ${GAIAT_CMD}
    DEPENDS ${ARG_DDL_FILE}
    DEPENDS ${ARG_RULESET_FILE}
  )

  set(GENERATE_RULES_TARGET "generate_${ARG_TARGET_NAME}")

  if(DEFINED ARG_PREVIOUS_TARGET_NAME)
    add_custom_target(${GENERATE_RULES_TARGET} ALL
      DEPENDS ${RULESET_CPP_OUT}
      DEPENDS ${EDC_LIBRARY}
      DEPENDS ${ARG_PREVIOUS_TARGET_NAME}
    )
  else()
    add_custom_target(${GENERATE_RULES_TARGET} ALL
      DEPENDS ${RULESET_CPP_OUT}
      DEPENDS ${EDC_LIBRARY}
    )
  endif()

  add_gtest(${ARG_TARGET_NAME}
    "${ARG_TARGET_SOURCES};${RULESET_CPP_OUT}"
    "${ARG_TARGET_INCLUDES};${EDC_INCLUDE}"
    "${ARG_TARGET_LIBRARIES}"
    "${GENERATE_RULES_TARGET}"
  )
endfunction()
