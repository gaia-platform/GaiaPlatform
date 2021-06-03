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

  if(NOT ("${ARGV6}" STREQUAL ""))
    set(ENV "${ARGV6}")
  else()
    set(ENV "")
  endif()

  set_target_properties(${TARGET} PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
  set_target_properties(${TARGET} PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
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

# Creates a CMake target that loads the DDL_FILE into the Gaia database,
# generates the EDC classes to access the database programmatically, and
# build the EDC classes into a static libary.
#
# The generated files are placed under: ${OUTPUT_FOLDER}/gaia_${DDL_NAME}.h
# where DDL_NAME is DDL_FILE with no extension.
#
# Args:
# - DDL_FILE: [optional] the path to the .ddl file.
#     If not provided will generate the EDC classes for DATABASE_NAME.
# - OUTPUT_FOLDER: [optional] folder where the header files will be generated.
#     If not provided the default value is ${GAIA_GENERATED_SCHEMAS}/${DATABASE_NAME}
# - LIB_NAME: [optional] the name of the generated target.
#     If not provided the default value is edc_${DDL_NAME}.
# - DATABASE_NAME: [optional] name of the database the headers are generated from.
#     If not provided the database name will be inferred as ${DDL_NAME}.
#     This is a temporary workaround, until we improve gaiac.
function(process_schema_internal)
  set(options "")
  set(oneValueArgs DDL_FILE OUTPUT_FOLDER LIB_NAME DATABASE_NAME INSTANCE_NAME)
  set(multiValueArgs "")
  cmake_parse_arguments("ARG" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT DEFINED ARG_DDL_FILE AND NOT DEFINED ARG_DATABASE_NAME)
      message(FATAL_ERROR "You must specify either the DDL_FILE or the DATABASE_NAME!")
  endif()

  # If the database name is not provided we infer it from the DDL file name.
  if(NOT DEFINED ARG_DATABASE_NAME)
    get_filename_component(DDL_NAME ${ARG_DDL_FILE} NAME)
    string(REPLACE ".ddl" "" DDL_NAME ${DDL_NAME})
    set(ARG_DATABASE_NAME ${DDL_NAME})
    message(VERBOSE "DATABASE_NAME not provided, inferred database name: ${ARG_DATABASE_NAME}.")
  endif()

  if(NOT DEFINED ARG_OUTPUT_FOLDER)
    set(ARG_OUTPUT_FOLDER ${GAIA_GENERATED_SCHEMAS}/${ARG_DATABASE_NAME})
    message(VERBOSE "OUTPUT_FOLDER not provided, defaulted to: ${ARG_OUTPUT_FOLDER}.")
  endif()

  set(EDC_HEADER_FILE ${ARG_OUTPUT_FOLDER}/gaia_${ARG_DATABASE_NAME}.h)
  set(EDC_CPP_FILE ${ARG_OUTPUT_FOLDER}/gaia_${ARG_DATABASE_NAME}.cpp)

  message(VERBOSE "Adding target to generate EDC classes for database ${ARG_DATABASE_NAME}.")

  string(RANDOM GAIAC_INSTANCE_NAME)

  if (DEFINED ARG_DDL_FILE)
    add_custom_command(
        COMMENT "Generating EDC classes for file ${ARG_DDL_FILE} and database ${ARG_DATABASE_NAME}..."
        OUTPUT ${EDC_HEADER_FILE}
        OUTPUT ${EDC_CPP_FILE}
        COMMAND ${GAIA_PROD_BUILD}/catalog/gaiac/gaiac
        -t ${GAIA_PROD_BUILD}/db/core
        -o ${ARG_OUTPUT_FOLDER}
        -g ${ARG_DDL_FILE}
        -d ${ARG_DATABASE_NAME}
        -n ${GAIAC_INSTANCE_NAME}
        DEPENDS ${ARG_DDL_FILE}
        DEPENDS gaiac
    )
  else()
    add_custom_command(
        COMMENT "Generating EDC classes for database ${ARG_DATABASE_NAME}..."
        OUTPUT ${EDC_HEADER_FILE}
        OUTPUT ${EDC_CPP_FILE}
        COMMAND ${GAIA_PROD_BUILD}/catalog/gaiac/gaiac
        -t ${GAIA_PROD_BUILD}/db/core
        -o ${ARG_OUTPUT_FOLDER}
        -d ${ARG_DATABASE_NAME}
        -n ${GAIAC_INSTANCE_NAME}
        -g
        DEPENDS gaiac
    )
  endif()

  if(NOT DEFINED ARG_LIB_NAME)
    set(ARG_LIB_NAME "edc_${ARG_DATABASE_NAME}")
    message(VERBOSE "LIB_NAME not provided, defaulting to: ${ARG_LIB_NAME}.")
  endif()

  add_library(${ARG_LIB_NAME}
      ${EDC_CPP_FILE})

  set_target_properties(${ARG_LIB_NAME} PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
  set_target_properties(${ARG_LIB_NAME} PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
  target_include_directories(${ARG_LIB_NAME} PUBLIC ${ARG_OUTPUT_FOLDER})
  target_include_directories(${ARG_LIB_NAME} PUBLIC ${FLATBUFFERS_INC})
  target_include_directories(${ARG_LIB_NAME} PRIVATE ${GAIA_INC})
  target_link_libraries(${ARG_LIB_NAME} gaia_direct)
endfunction()
