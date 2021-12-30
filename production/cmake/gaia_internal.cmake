#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# The user must explicitly set a C++ standard version; to avoid confusion, we
# do not fall back to a default version.
if(NOT CMAKE_CXX_STANDARD)
  message(FATAL_ERROR "CMAKE_CXX_STANDARD is not set!")
endif()

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
# This function only exists because CMake symbol visibility properties
# (CXX_VISIBILITY_PRESET/VISIBILITY_INLINES_HIDDEN) don't seem to propagate to
# dependent targets when they're set on an INTERFACE target (i.e.,
# gaia_build_options), so we need to set them directly on the target.
#
function(configure_gaia_target TARGET)
  # Keep this dependency PRIVATE to avoid leaking Gaia build options into all dependent targets.
  target_link_libraries(${TARGET} PRIVATE gaia_build_options)
  if(NOT EXPORT_SYMBOLS)
    # See https://cmake.org/cmake/help/latest/policy/CMP0063.html.
    cmake_policy(SET CMP0063 NEW)
    # This property sets the compiler option -fvisibility=hidden, so all symbols
    # are "hidden" (i.e., not exported) from our shared library (libgaia.so).
    # See https://gcc.gnu.org/wiki/Visibility.
    set_target_properties(${TARGET} PROPERTIES CXX_VISIBILITY_PRESET hidden)
    # This property sets the compiler option -fvisibility-inlines-hidden.
    # "This causes all inlined class member functions to have hidden visibility"
    # (https://gcc.gnu.org/wiki/Visibility).
    set_target_properties(${TARGET} PROPERTIES VISIBILITY_INLINES_HIDDEN ON)
  endif(NOT EXPORT_SYMBOLS)
endfunction(configure_gaia_target)

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

  # REVIEW: We don't currently expose a way to mark passed include dirs as
  # SYSTEM (to suppress warnings), so we just treat all of them as such.
  target_include_directories(${TARGET} SYSTEM PRIVATE ${INCLUDES} ${GOOGLE_TEST_INC})
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

  if("$CACHE{SANITIZER}" STREQUAL "ASAN")
    # Suppress ASan warnings from exception destructors in libc++.
    # REVIEW (GAIAPLAT-1828): spdlog and cpptoml show up in the ASan stack
    # trace, and both are unconditionally built with libstdc++, so this is
    # likely an ABI incompatibility with libc++.
    # NB: This overwrites any previous value of ENV, but apparently we're not
    # using ENV for anything, and I couldn't get concatenation of NAME=VALUE
    # env var pairs to work with ASan. This is just a temporary hack anyway.
    set(ENV "ASAN_OPTIONS=alloc_dealloc_mismatch=0")
  endif()

  if("$CACHE{SANITIZER}" STREQUAL "TSAN")
    # NB: This overwrites any previous value of ENV, but apparently we're not
    # using ENV for anything.
    set(ENV "TSAN_OPTIONS=suppressions=${GAIA_REPO}/.tsan-suppressions")
  endif()

  configure_gaia_target(${TARGET})
  set_target_properties(${TARGET} PROPERTIES CXX_CLANG_TIDY "")
  add_dependencies(${TARGET} gaia_db_server_exec)
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
# into the Gaia database. It then generates the DAC code to access the
# database programmatically, and build the DAC classes into a static library.
#
# The generated file is written to: ${OUTPUT_FOLDER}/gaia_${DDL_NAME}.h
# where DDL_NAME is DDL_FILE with no extension.
#
# Args:
# - DDL_FILE: [optional] the path to the .ddl file.
#     If not specified, the function generates the DAC code for the database
#     specified by DATABASE_NAME.
# - LIB_NAME: [optional] the name of the generated target.
#     If not specified the default value is dac_${DDL_NAME}.
# - DATABASE_NAME: [optional] name of the database the headers are generated from.
#     If not specified, the default database will be used.
function(process_schema_internal)
  set(options "")
  set(oneValueArgs DDL_FILE LIB_NAME DATABASE_NAME INSTANCE_NAME)
  set(multiValueArgs "")
  cmake_parse_arguments("ARG" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT DEFINED ARG_DDL_FILE AND NOT DEFINED ARG_DATABASE_NAME)
    message(FATAL_ERROR "You must specify either the DDL_FILE or the DATABASE_NAME!")
  endif()

  set(ARG_OUTPUT_DIR ${GAIA_GENERATED_CODE}/direct_access/${ARG_DATABASE_NAME})
  file(MAKE_DIRECTORY ${ARG_OUTPUT_DIR})

  string(RANDOM DB_INSTANCE_NAME)

  set(GAIAC_COMMAND ${GAIA_PROD_BUILD}/catalog/gaiac/gaiac)
  set(GAIAC_ARGS "-t" "${GAIA_PROD_BUILD}/db/core" "-o" "${ARG_OUTPUT_DIR}" "-n" "${DB_INSTANCE_NAME}" "-g")

  if (DEFINED ARG_DDL_FILE)
    message(STATUS "Loading schema from the DDL file ${ARG_DDL_FILE}...")
    list(APPEND GAIAC_ARGS ${ARG_DDL_FILE})
  endif()

  if(DEFINED ARG_DATABASE_NAME)
    set(DAC_HEADER_FILE ${ARG_OUTPUT_DIR}/gaia_${ARG_DATABASE_NAME}.h)
    set(DAC_CPP_FILE ${ARG_OUTPUT_DIR}/gaia_${ARG_DATABASE_NAME}.cpp)
    message(STATUS "Generating DAC code for database ${ARG_DATABASE_NAME}...")
    list(PREPEND GAIAC_ARGS "-d" "${ARG_DATABASE_NAME}")
  else()
    # If the database name is not specified, we use the default database.
    message(STATUS "DATABASE_NAME not specified, using default.")
    set(DAC_HEADER_FILE ${ARG_OUTPUT_DIR}/gaia.h)
    set(DAC_CPP_FILE ${ARG_OUTPUT_DIR}/gaia.cpp)
    message(VERBOSE "Generating DAC code for the default database...")
  endif()

  add_custom_command(
    COMMENT "Generating DAC code in ${ARG_OUTPUT_DIR}..."
    OUTPUT ${DAC_HEADER_FILE}
    OUTPUT ${DAC_CPP_FILE}
    COMMAND ${GAIAC_COMMAND} ${GAIAC_ARGS}
    DEPENDS ${ARG_DDL_FILE}
    DEPENDS gaiac
  )

  if(NOT DEFINED ARG_LIB_NAME)
    if (DEFINED ARG_DATABASE_NAME)
      set(ARG_LIB_NAME "dac_${ARG_DATABASE_NAME}")
    else()
      set(ARG_LIB_NAME "dac")
    endif()
    message(VERBOSE "DAC LIB_NAME not specified, using: ${ARG_LIB_NAME}.")
  endif()

  add_library(${ARG_LIB_NAME}
    ${DAC_CPP_FILE})

  configure_gaia_target(${ARG_LIB_NAME})
  target_include_directories(${ARG_LIB_NAME} PUBLIC ${ARG_OUTPUT_DIR})
  target_include_directories(${ARG_LIB_NAME} PUBLIC ${FLATBUFFERS_INC})
  target_include_directories(${ARG_LIB_NAME} PRIVATE ${GAIA_INC})
  target_link_libraries(${ARG_LIB_NAME} PUBLIC gaia_direct)

  # Add the DDL arg file
  if (DEFINED ARG_DDL_FILE)
    get_filename_component(DDL_FILE_ABSOLUTE_PATH ${ARG_DDL_FILE} ABSOLUTE)
    set_target_properties(${ARG_LIB_NAME} PROPERTIES DDL_FILE ${DDL_FILE_ABSOLUTE_PATH})
  endif()
endfunction()

# Creates a CMake target that translates the ruleset file specified by RULESET_FILE
# and writes the translated rules as a cpp file.
# The generated cpp file is written to the directory specified by OUTPUT_DIR with the
# name ${RULESET_NAME}_ruleset.cpp, where RULESET_NAME is RULESET_FILE with no extension.
#
# This function tries to infer some of the gaiat parameters such as:
# - The default C++ include path.
# - The Gaia path.
# - The C++ version.
#
# Args:
# - RULESET_FILE: the path to the .ruleset file.
# - LIB_NAME: [optional] the name of the generated target.
#     If not specified the default value is ${RULESET_NAME}_ruleset.
# - CLANG_PARAMS: [optional]: Additional parameters to pass to clang (invoked by gaiat)
# - DEPENDS: [optional] an optional list of targets on which this task depends.
#     Typically, the translation depends on the successful generation of the
#     schema headers.
#     If not specified, translation will depend on the targets listed in
#     ${GAIA_DIRECT_ACCESS_GENERATION_TARGETS}
function(translate_ruleset_internal)
  set(options "")
  set(oneValueArgs RULESET_FILE LIB_NAME TARGET_NAME DAC_LIBRARY GAIAT_CMD)
  set(multiValueArgs CLANG_PARAMS DEPENDS)
  cmake_parse_arguments("ARG" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  check_param(ARG_RULESET_FILE)

  get_filename_component(RULESET_NAME ${ARG_RULESET_FILE} NAME)
  string(REPLACE ".ruleset" "" RULESET_NAME ${RULESET_NAME})

  set(ARG_OUTPUT_DIR ${GAIA_GENERATED_CODE}/rules/${RULESET_NAME})
  file(MAKE_DIRECTORY ${ARG_OUTPUT_DIR})

  set(RULESET_CPP_NAME ${RULESET_NAME}_ruleset.cpp)
  set(RULESET_CPP_PATH ${ARG_OUTPUT_DIR}/${RULESET_CPP_NAME})

  message(STATUS "Translating ruleset: ${ARG_RULESET_FILE} into ${RULESET_CPP_NAME}...")

  string(RANDOM DB_INSTANCE_NAME)

  set(GAIAT_INCLUDE_PATH "")

  # Add implicit include directories
  foreach(INCLUDE_PATH ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
    # Have to use ; instead of space otherwise custom_command will try to escape it
    string(APPEND GAIAT_INCLUDE_PATH "-I;${INCLUDE_PATH};")
  endforeach()

  # Add default Gaia path
  string(APPEND GAIAT_INCLUDE_PATH "-I;${GAIA_INC};")

  set(GAIAT_CMD "${GAIA_PROD_BUILD}/tools/gaia_translate/gaiat")
  set(GAIAC_CMD ${GAIA_PROD_BUILD}/catalog/gaiac/gaiac)

  # Unlike clang, gaiat isn't smart enough to know where system include dirs are
  # for intrinsics and stdlib headers, so we need to define them explicitly.
  set(GAIAT_INCLUDE_PATH "")

  # We use libc++ in debug and its header must be manually included.
  # Note: the order of inclusion is relevant and libc++ headers must be
  # defined first when libc++ is used.
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(LIBCXX_INCLUDE_DIR "/usr/lib/llvm-13/include/c++/v1/")
    # Have to use ; instead of space otherwise custom_command will try to escape it
    string(APPEND GAIAT_INCLUDE_PATH "-I;${LIBCXX_INCLUDE_DIR};")
  endif()

  foreach(INCLUDE_PATH ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
    string(APPEND GAIAT_INCLUDE_PATH "-I;${INCLUDE_PATH};")
  endforeach()

  # Get the include directories from the DAC_LIBRARY target.
  get_target_property(DAC_INCLUDE ${ARG_DAC_LIBRARY} INCLUDE_DIRECTORIES)

  foreach(INCLUDE_PATH ${DAC_INCLUDE})
    string(APPEND GAIAT_INCLUDE_PATH "-I;${INCLUDE_PATH};")
  endforeach()

  get_target_property(DDL_FILE ${ARG_DAC_LIBRARY} DDL_FILE)

  add_custom_command(
      COMMENT "Translating ${ARG_RULESET_FILE} into ${RULESET_CPP_NAME}..."
      OUTPUT ${RULESET_CPP_PATH}
      COMMAND ${GAIA_PROD_BUILD}/db/core/gaia_db_server --persistence disabled --instance-name ${DB_INSTANCE_NAME} &
      COMMAND sleep 1
      COMMAND ${GAIAC_CMD} ${DDL_FILE} -n ${DB_INSTANCE_NAME}
      COMMAND ${GAIAT_CMD} ${ARG_RULESET_FILE} -output ${RULESET_CPP_PATH} -n ${DB_INSTANCE_NAME} --
      -I ${GAIA_INC}
      -I ${FLATBUFFERS_INC}
      -I ${GAIA_SPDLOG_INC}
      -I ${GAIAT_INCLUDE_PATH}
      -stdlib=$<IF:$<CONFIG:Debug>,libc++,libstdc++>
      -std=c++${CMAKE_CXX_STANDARD}
      # Kill gaia_db_server by matching the instance name. --oldest is necessary to prevent
      # pgrep from matching itself. The idea is that the PID of gaia_db_server should start
      # earlier than pgrep hence having a lower PID.
      COMMAND kill -9 `pgrep --oldest --full ${DB_INSTANCE_NAME}`

      # In some contexts, the next attempt to start gaia_db_server precedes this kill, leading
      # to a build failure. A short sleep is currently fixing that, but may not be the
      # correct long-term solution.
      COMMAND sleep 1
      DEPENDS ${GAIAC_CMD}
      DEPENDS ${GAIAT_CMD}
      DEPENDS ${ARG_DAC_LIBRARY}
      DEPENDS ${ARG_RULESET_FILE}
  )

  if(NOT DEFINED ARG_LIB_NAME)
    set(ARG_LIB_NAME "${RULESET_NAME}_ruleset")
    message(VERBOSE "Ruleset LIB_NAME not specified, using: ${ARG_LIB_NAME}.")
  endif()

  add_library(${ARG_LIB_NAME}
      ${RULESET_CPP_PATH})

  configure_gaia_target(${ARG_LIB_NAME})
  target_include_directories(${ARG_LIB_NAME} PRIVATE ${FLATBUFFERS_INC})
  target_include_directories(${ARG_LIB_NAME} PRIVATE ${GAIA_INC})
  target_link_libraries(${ARG_LIB_NAME} PUBLIC gaia_direct ${ARG_DAC_LIBRARY})
endfunction()


# Stop CMake if the given parameter was not passed to the function.
macro(check_param PARAM)
  if(NOT DEFINED ${PARAM})
    message(FATAL_ERROR "The parameter ${PARAM} is required!")
  endif()
endmacro()
