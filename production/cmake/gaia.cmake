cmake_minimum_required(VERSION 3.16)

# Sets Gaia variables. They can be overridden to customize the Gaia behavior.
#
# The script searches for the 'gaia' library. If it does not find it,
# it tries to build it ${GAIA_LIB}. If ${GAIA_LIB} does not exist contain
# libgaia.so the script exits with an error.
#
# - GAIA_ROOT: Gaia root directory. Default: /opt/gaia.
# - GAIA_INC: Gaia include directory. Default: ${GAIA_ROOT}/include.
# - GAIA_BIN: Gaia bin directory. Default: ${GAIA_ROOT}/bin.
# - GAIA_LIB_DIR: libgaia.so directory. Default: ${GAIA_ROOT}/lib.
# - GAIA_LIB: Automatically set by CMake if libgaia.so is found.
# - GAIA_GAIAC_CMD: The gaiac command. Default: ${GAIA_BIN}/gaiac.
# - GAIA_GAIAT_CMD: The gaiat command. Default: ${GAIA_BIN}/gaiat.
# - GAIA_DEFAULT_EDC_GENERATED_DIR: The process_schema() function puts the generated EDC code
#     in this directory if the OUTPUT_DIR param is not specified.
# - GAIA_DEFAULT_RULES_GENERATED_DIR: The translate_ruleset() function puts the generated rules code
#     in this directory if the OUTPUT_DIR param is not specified.

if (NOT DEFINED GAIA_ROOT)
  set(GAIA_ROOT "/opt/gaia")
endif()

if (NOT DEFINED GAIA_INC)
  set(GAIA_INC "${GAIA_ROOT}/include")
endif()

if (NOT DEFINED GAIA_BIN)
  set(GAIA_BIN "${GAIA_ROOT}/bin")
endif()

if (NOT DEFINED GAIA_LIB_DIR)
  set(GAIA_LIB_DIR "${GAIA_ROOT}/lib")
endif()

if (NOT DEFINED GAIA_GAIAC_CMD)
  set(GAIA_GAIAC_CMD "${GAIA_BIN}/gaiac")
endif()

if (NOT DEFINED GAIA_GAIAT_CMD)
  set(GAIA_GAIAT_CMD "${GAIA_BIN}/gaiat")
endif()

find_library(GAIA_LIB gaia)
if(NOT GAIA_LIB)
  if (NOT EXISTS ${GAIA_LIB_DIR}/libgaia.so)
    message(FATAL_ERROR "${GAIA_LIB_DIR}/libgaia.so not found")
  endif()
  add_library(gaia SHARED IMPORTED GLOBAL)
  set_target_properties(gaia PROPERTIES IMPORTED_LOCATION ${GAIA_LIB_DIR}/libgaia.so)
endif()

if (NOT DEFINED GAIA_DEFAULT_EDC_GENERATED_DIR)
  set(GAIA_DEFAULT_EDC_GENERATED_DIR ${CMAKE_BINARY_DIR}/gaia_generated/edc)
  file(MAKE_DIRECTORY ${GAIA_DEFAULT_EDC_GENERATED_DIR})
endif()

if (NOT DEFINED GAIA_DEFAULT_RULES_GENERATED_DIR)
  set(GAIA_DEFAULT_RULES_GENERATED_DIR ${CMAKE_BINARY_DIR}/gaia_generated/rules)
  file(MAKE_DIRECTORY ${GAIA_DEFAULT_RULES_GENERATED_DIR})
endif()

# The target_add_gaia_generated_sources() function uses the following variables
# to add the generated sources to a user target. The process_schema() and
# translate_ruleset() functions set the value of these variables after generating the code.
list(APPEND GAIA_EDC_GENERATION_TARGETS "")
list(APPEND GAIA_EDC_GENERATED_HEADERS "")
list(APPEND GAIA_EDC_GENERATED_CPP "")

list(APPEND GAIA_RULES_TRANSLATION_TARGETS "")
list(APPEND GAIA_RULES_TRANSLATED_CPP "")

# Stop CMake if the given parameter was not passed to the function.
macro(check_param PARAM)
  if(NOT DEFINED ${PARAM})
    message(FATAL_ERROR "The parameter ${PARAM} is required!")
  endif()
endmacro()

# Creates a CMake target that generates the EDC classes for a given Gaia database.
# Optionally loads a DDL file into the database before the generation.
#
# The path for the generated files is as follows: ${OUTPUT_DIR}/gaia_${DDL_NAME}[.h|.cpp]
# where DDL_NAME is DDL_FILE with no extension.
#
# Args:
# - DATABASE_NAME: [optional] name of the database the code is generated from.
#     If not specified, the database name is inferred as ${DDL_NAME}.
#     Changes to this functionality are under review.
# - DDL_FILE: [optional] the path to the .ddl file to load into the database before the generation.
# - OUTPUT_DIR: [optional] directory where the header files will be generated.
#     If an output folder is not specified, the files are written to ${GAIA_DEFAULT_EDC_GENERATED_DIR}/${DATABASE_NAME}.
# - INSTANCE_NAME: [optional] name of the database instance gaiac will connect to.
#     If not specified it will try to connect to gaia_default_instance.
# - TARGET_NAME: [optional] the name of the generated target.
#     If not specified the default value is generate_${DDL_NAME}_edc.
function(process_schema)
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
    message(STATUS "DATABASE_NAME not specified, using: ${ARG_DATABASE_NAME}.")
  endif()

  if(NOT DEFINED ARG_OUTPUT_DIR)
    set(ARG_OUTPUT_DIR ${GAIA_DEFAULT_EDC_GENERATED_DIR}/${ARG_DATABASE_NAME})
    message(STATUS "OUTPUT_DIR not specified, using: ${ARG_OUTPUT_DIR}.")
  endif()

  set(EDC_HEADER_FILE ${ARG_OUTPUT_DIR}/gaia_${ARG_DATABASE_NAME}.h)
  set(EDC_CPP_FILE ${ARG_OUTPUT_DIR}/gaia_${ARG_DATABASE_NAME}.cpp)

  if (DEFINED ARG_INSTANCE_NAME)
    set(INSTANCE_NAME "-n ${ARG_INSTANCE_NAME}")
  endif()

  message(STATUS "Adding target to generate EDC code for database ${ARG_DATABASE_NAME}...")

  if (DEFINED ARG_DDL_FILE)
    add_custom_command(
      COMMENT "Generating EDC code for database ${ARG_DATABASE_NAME} into ${ARG_OUTPUT_DIR}..."
      OUTPUT ${EDC_HEADER_FILE}
      OUTPUT ${EDC_CPP_FILE}
      COMMAND ${GAIA_GAIAC_CMD}
        -o ${ARG_OUTPUT_DIR}
        -d ${ARG_DATABASE_NAME}
        ${ARG_INSTANCE_NAME}
        -g ${ARG_DDL_FILE}
      DEPENDS ${ARG_DDL_FILE}
    )
  else()
    add_custom_command(
      COMMENT "Generating EDC code for database ${ARG_DATABASE_NAME} into ${ARG_OUTPUT_DIR}..."
      OUTPUT ${EDC_HEADER_FILE}
      OUTPUT ${EDC_CPP_FILE}
      COMMAND ${GAIA_GAIAC_CMD}
        -o ${ARG_OUTPUT_DIR}
        -d ${ARG_DATABASE_NAME}
        ${ARG_INSTANCE_NAME}
        -g
      DEPENDS ${ARG_DDL_FILE}
    )
  endif()

  if(NOT DEFINED ARG_TARGET_NAME)
    set(ARG_TARGET_NAME "generate_${DDL_NAME}_edc")
    message(STATUS "TARGET_NAME not specified, using default value: ${ARG_TARGET_NAME}.")
  endif()

  add_custom_target(${ARG_TARGET_NAME} ALL
    DEPENDS ${EDC_HEADER_FILE}
    DEPENDS ${EDC_CPP_FILE}
  )

  set(GAIA_EDC_GENERATION_TARGETS "${GAIA_EDC_GENERATION_TARGETS};${ARG_TARGET_NAME}" PARENT_SCOPE)
  set(GAIA_EDC_GENERATED_HEADERS "${GAIA_EDC_GENERATED_HEADERS};${EDC_HEADER_FILE}" PARENT_SCOPE)
  set(GAIA_EDC_GENERATED_CPP "${GAIA_EDC_GENERATED_CPP};${EDC_CPP_FILE}" PARENT_SCOPE)
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
# - OUTPUT_DIR: [optional] directory where the header files will be generated.
#     If not specified the default value is ${GAIA_DEFAULT_EDC_GENERATED_DIR}/${RULESET_NAME}
# - TARGET_NAME: [optional] the name of the generated target.
#     If not specified the default value is translate_${RULESET_NAME}_ruleset.
# - CLANG_PARAMS: [optional]: Additional parameters to pass to clang (invoked by gaiat)
# - INSTANCE_NAME: [optional] name of the database instance gaiat will connect to.
#     If not specified it will try to connect to gaia_default_instance.
# - DEPENDS: [optional] an optional list of targets on which this task depends.
#     Typically, the translation depends on the successful generation of the
#     schema headers.
#     If not specified, translation will depend on the targets listed in
#     ${GAIA_EDC_GENERATION_TARGETS}
function(translate_ruleset)
  set(options "")
  set(oneValueArgs RULESET_FILE OUTPUT_DIR TARGET_NAME GAIAT_CMD)
  set(multiValueArgs CLANG_PARAMS DEPENDS)
  cmake_parse_arguments("ARG" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  check_param(ARG_RULESET_FILE)

  get_filename_component(RULESET_NAME ${ARG_RULESET_FILE} NAME)
  string(REPLACE ".ruleset" "" RULESET_NAME ${RULESET_NAME})

  if(NOT DEFINED ARG_OUTPUT_DIR)
    set(ARG_OUTPUT_DIR ${GAIA_DEFAULT_RULES_GENERATED_DIR}/${RULESET_NAME})
    message(STATUS "OUTPUT_DIR not specified, using: ${ARG_OUTPUT_DIR}.")
    file(MAKE_DIRECTORY ${ARG_OUTPUT_DIR})
  endif()

  set(RULESET_CPP_NAME ${RULESET_NAME}_ruleset.cpp)
  set(RULESET_CPP_PATH ${ARG_OUTPUT_DIR}/${RULESET_CPP_NAME})

  message(STATUS "Adding target for translating ruleset: ${ARG_RULESET_FILE} into ${RULESET_CPP_NAME}...")

  set(GAIAT_INCLUDE_PATH "")

  # Add implicit include directories
  foreach(INCLUDE_PATH ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
    # Have to use ; instead of space otherwise custom_command will try to escape it
    string(APPEND GAIAT_INCLUDE_PATH "-I;${INCLUDE_PATH};")
  endforeach()

  # Add default Gaia path
  string(APPEND GAIAT_INCLUDE_PATH "-I;${GAIA_INC};")

  # Add the output directory (which contains the DDL headers)

  foreach(HEADER_FILE ${GAIA_EDC_GENERATED_HEADERS})
    get_filename_component(HEADER_DIR ${HEADER_FILE} DIRECTORY)
    message(STATUS "Adding ${HEADER_DIR} to ${ARG_TARGET_NAME}...")
    string(APPEND GAIAT_INCLUDE_PATH "-I;${HEADER_DIR};")
  endforeach()

  if (DEFINED ARG_INSTANCE_NAME)
      set(INSTANCE_NAME "-n ${ARG_INSTANCE_NAME}")
  endif()

  if(NOT DEFINED ARG_DEPENDS)
    set(ARG_DEPENDS ${GAIA_EDC_GENERATION_TARGETS})
  endif()

  add_custom_command(
    COMMENT "Translating ${ARG_RULESET_FILE} into ${RULESET_CPP_NAME}..."
    OUTPUT ${RULESET_CPP_PATH}
    COMMAND ${GAIA_GAIAT_CMD} ${ARG_RULESET_FILE} -output ${RULESET_CPP_PATH} ${INSTANCE_NAME} --
      ${ARG_CLANG_PARAMS}
      ${GAIAT_INCLUDE_PATH}
      -std=c++${CMAKE_CXX_STANDARD}
    DEPENDS ${ARG_RULESET_FILE} ${ARG_DEPENDS}
  )

  if(NOT DEFINED ARG_TARGET_NAME)
    set(ARG_TARGET_NAME "translate_${RULESET_NAME}_ruleset")
    message(STATUS "TARGET_NAME not specified, using default value: ${ARG_TARGET_NAME}.")
  endif()

  add_custom_target(${ARG_TARGET_NAME} ALL
    DEPENDS ${RULESET_CPP_PATH})

  set(GAIA_RULES_TRANSLATION_TARGETS "${GAIA_RULES_TRANSLATION_TARGETS};${ARG_TARGET_NAME}" PARENT_SCOPE)
  set(GAIA_RULES_TRANSLATED_CPP "${GAIA_RULES_TRANSLATED_CPP};${RULESET_CPP_PATH}" PARENT_SCOPE)
endfunction()

# Adds the Gaia generated files (EDC and Rules) to the given target.
#
# This function depends on the output of the process_schema() and
# translate_ruleset() functions.
function(target_add_gaia_generated_sources TARGET_NAME)
  # Adds EDC .h header directories
  foreach(HEADER_FILE ${GAIA_EDC_GENERATED_HEADERS})
    get_filename_component(HEADER_DIR ${HEADER_FILE} DIRECTORY)
    message(STATUS "Adding ${HEADER_DIR} to ${TARGET_NAME}...")
    target_include_directories(${TARGET_NAME} PUBLIC ${HEADER_DIR})
  endforeach()

  # Adds EDC .cpp files
  foreach(CPP_FILE ${GAIA_EDC_GENERATED_CPP})
    message(STATUS "Adding ${CPP_FILE} to ${TARGET_NAME}...")
    target_sources(${TARGET_NAME} PRIVATE ${CPP_FILE})
  endforeach()

  # Adds Rules .cpp files
  foreach(CPP_FILE ${GAIA_RULES_TRANSLATED_CPP})
    message(STATUS "Adding ${CPP_FILE} to ${TARGET_NAME}...")
    target_sources(${TARGET_NAME} PRIVATE ${CPP_FILE})
  endforeach()
endfunction()
