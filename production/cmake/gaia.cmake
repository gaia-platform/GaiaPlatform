cmake_minimum_required(VERSION 3.16)

# Stop CMake if the given parameter was not passed to the function.
macro(check_param PARAM)
  if(NOT DEFINED ${PARAM})
    message(FATAL_ERROR "The parameter ${PARAM} is required!")
  endif()
endmacro()

# Creates a CMake target that loads the DDL_FILE into the Gaia database
# and generates the EDC headers to access the database programmatically.
# The generated file is placed inside OUTPUT_FOLDER with the name
# gaia_${DDL_NAME}.h where DDL_NAME is DDL_FILE with no extension.
#
# Args:
# - DDL_FILE: the path to the .ddl file.
# - OUTPUT_FOLDER: folder where the header files will be generated.
# - TARGET_NAME: [optional] the name of the generated target. If not provided
#                the default value is generate_${DDL_NAME}_headers.
# - GAIAC_CMD: [optional] custom gaiac command. If not provided will search gaiac
#              in the path.
function(process_schema)
  set(options "")
  set(oneValueArgs DDL_FILE OUTPUT_FOLDER TARGET_NAME GAIAC_CMD)
  set(multiValueArgs "")
  cmake_parse_arguments("ARG" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  check_param(ARG_DDL_FILE)
  check_param(ARG_OUTPUT_FOLDER)

  get_filename_component(DDL_NAME ${ARG_DDL_FILE} NAME)
  string(REPLACE ".ddl" "" DDL_NAME ${DDL_NAME})
  set(SCHEMA_HEADER_PATH ${ARG_OUTPUT_FOLDER}/gaia_${DDL_NAME}.h)

  message(VERBOSE "Adding target for generating: ${ARG_DDL_FILE}.")

  if(NOT DEFINED ARG_GAIAC_CMD)
    set(ARG_GAIAC_CMD gaiac)
  endif()

  add_custom_command(
    COMMENT "Generating ${DDL_NAME}.h..."
    OUTPUT ${SCHEMA_HEADER_PATH}
    COMMAND ${ARG_GAIAC_CMD} -o ${ARG_OUTPUT_FOLDER} -g ${ARG_DDL_FILE}
    DEPENDS ${ARG_DDL_FILE}
  )

  if(NOT DEFINED ARG_TARGET_NAME)
    set(ARG_TARGET_NAME "generate_${DDL_NAME}_headers")
    message(VERBOSE "TARGET_NAME not provided, using default value: ${ARG_TARGET_NAME}.")
  endif()

  add_custom_target(${ARG_TARGET_NAME} ALL
    DEPENDS ${SCHEMA_HEADER_PATH})
endfunction()

# Creates a CMake target that translate the RULESET_FILE into a cpp file.
# The generated cpp file is placed inside OUTPUT_FOLDER with the name
# (${RULESET_NAME}_ruleset.cpp), where RULESET_NAME is RULESET_FILE with
# no extension.
#
# This function tries to infer some of the gaiat parameters such as:
# - The default C++ include path.
# - The Gaia path.
# - The C++ version.
#
# Args:
# - RULESET_FILE: the path to the .ruleset file.
# - OUTPUT_FOLDER: folder where the .cpp files will be generated.
# - TARGET_NAME: [optional] the name of the generated target. If not provided
#                the default value is translate_${RULESET_NAME}_ruleset.
# - CLANG_PARAMS: [optional]: Additional parameters to pass to clang (invoked by gaiat)
# - GAIAT_CMD: [optional] custom gaiac command. If not provided will search gaiac
#              in the path.
# - DEPENDS: [optional] optional list of targets this task depends on.
#            Typically the translation has to depend on the generation of the
#            schema headers.
function(translate_ruleset)
  set(options "")
  set(oneValueArgs RULESET_FILE OUTPUT_FOLDER TARGET_NAME GAIAT_CMD DEPENDS)
  set(multiValueArgs CLANG_PARAMS)
  cmake_parse_arguments("ARG" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  check_param(ARG_RULESET_FILE)
  check_param(ARG_OUTPUT_FOLDER)

  get_filename_component(RULESET_NAME ${ARG_RULESET_FILE} NAME)
  string(REPLACE ".ruleset" "" RULESET_NAME ${RULESET_NAME})
  set(RULESET_CPP_NAME ${RULESET_NAME}_ruleset.cpp)
  set(RULESET_CPP_PATH ${ARG_OUTPUT_FOLDER}/${RULESET_CPP_NAME})

  message(VERBOSE "Adding target for translating ruleset: ${ARG_RULESET_FILE} into ${RULESET_CPP_NAME}.")

  set(GAIAT_INCLUDE_PATH "")

  # Add implicit include directories
  foreach(INCLUDE_PATH ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
    # Have to use ; instead of space otherwise custom_command will try to escape it
    string(APPEND GAIAT_INCLUDE_PATH "-I;${INCLUDE_PATH};")
  endforeach()

  # Add default Gaia path
  string(APPEND GAIAT_INCLUDE_PATH "-I;/opt/gaia/include;")

  # Add the output folder (which contains the DDL headers)
  string(APPEND GAIAT_INCLUDE_PATH "-I;${ARG_OUTPUT_FOLDER};")

  if(NOT DEFINED ARG_GAIAT_CMD)
    set(ARG_GAIAT_CMD gaiat)
  endif()

  add_custom_command(
    COMMENT "Translating ${ARG_RULESET_FILE} into ${RULESET_CPP_NAME}..."
    OUTPUT ${RULESET_CPP_PATH}
    COMMAND ${ARG_GAIAT_CMD} ${ARG_RULESET_FILE} -output ${RULESET_CPP_PATH} --
    ${ARG_CLANG_PARAMS}
    ${GAIAT_INCLUDE_PATH}
    -std=c++${CMAKE_CXX_STANDARD}
    DEPENDS ${ARG_RULESET_FILE} ${ARG_DEPENDS}
  )

  if(NOT DEFINED ARG_TARGET_NAME)
    set(ARG_TARGET_NAME "translate_${RULESET_NAME}_ruleset")
    message(VERBOSE "TARGET_NAME not provided, using default value: ${ARG_TARGET_NAME}.")
  endif()

  add_custom_target(${ARG_TARGET_NAME} ALL
    DEPENDS ${RULESET_CPP_PATH})
endfunction()
