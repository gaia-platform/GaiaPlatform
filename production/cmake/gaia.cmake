cmake_minimum_required(VERSION 3.16)

if (NOT DEFINED GAIA_GAIAC_CMD)
  set(GAIA_GAIAC_CMD gaiac)
endif()

if (NOT DEFINED GAIA_GAIAT_CMD)
  set(GAIA_GAIAT_CMD gaiat)
endif()

if (NOT DEFINED GAIA_DEFAULT_EDC_GENERATED_DIR)
  set(GAIA_DEFAULT_EDC_GENERATED_DIR ${CMAKE_BINARY_DIR}/gaia_generated/edc)
  file(MAKE_DIRECTORY ${GAIA_DEFAULT_EDC_GENERATED_DIR})
endif()

if (NOT DEFINED GAIA_DEFAULT_RULES_GENERATED_DIR)
  set(GAIA_DEFAULT_RULES_GENERATED_DIR ${CMAKE_BINARY_DIR}/gaia_generated/rules)
  file(MAKE_DIRECTORY ${GAIA_DEFAULT_RULES_GENERATED_DIR})
endif()

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
# The generated files are placed under: ${OUTPUT_FOLDER}/gaia_${DDL_NAME}.h
# where DDL_NAME is DDL_FILE with no extension.
#
# Args:
# - DATABASE_NAME: [optional] name of the database the code is generated from.
#     If not provided the database name will be inferred as ${DDL_NAME}.
#     This is a temporary workaround, until we improve gaiac.
# - DDL_FILE: [optional] the path to the .ddl file.
#     If not provided will generate the EDC classes for DATABASE_NAME.
# - OUTPUT_FOLDER: [optional] folder where the header files will be generated.
#     If not provided the default value is ${GAIA_DEFAULT_EDC_GENERATED_DIR}/${DATABASE_NAME}
# - INSTANCE_NAME: [optional] name of the database instance gaiac will connect to.
# - TARGET_NAME: [optional] the name of the generated target.
#     If not provided the default value is generate_edc_${DDL_NAME}.
function(process_schema)
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
    message(STATUS "DATABASE_NAME not provided, inferred database name: ${ARG_DATABASE_NAME}.")
  endif()

  if(NOT DEFINED ARG_OUTPUT_FOLDER)
    set(ARG_OUTPUT_FOLDER ${GAIA_DEFAULT_EDC_GENERATED_DIR}/${ARG_DATABASE_NAME})
    message(STATUS "OUTPUT_FOLDER not provided, using: ${ARG_OUTPUT_FOLDER}.")
  endif()

  set(EDC_HEADER_FILE ${ARG_OUTPUT_FOLDER}/gaia_${ARG_DATABASE_NAME}.h)
  set(EDC_CPP_FILE ${ARG_OUTPUT_FOLDER}/gaia_${ARG_DATABASE_NAME}.cpp)

  if (DEFINED ARG_INSTANCE_NAME)
    set(INSTANCE_NAME "-n ${ARG_INSTANCE_NAME}")
  endif()

  message(STATUS "Adding target to generate EDC classes for database ${ARG_DATABASE_NAME}.")

  if (DEFINED ARG_DDL_FILE)
    add_custom_command(
        COMMENT "Generating EDC classes for database ${ARG_DATABASE_NAME} into ${ARG_OUTPUT_FOLDER}..."
        OUTPUT ${EDC_HEADER_FILE}
        OUTPUT ${EDC_CPP_FILE}
        COMMAND ${GAIA_GAIAC_CMD}
        -o ${ARG_OUTPUT_FOLDER}
        -d ${ARG_DATABASE_NAME}
        ${ARG_INSTANCE_NAME}
        -g ${ARG_DDL_FILE}
        DEPENDS ${ARG_DDL_FILE}
    )
  else()
    add_custom_command(
        COMMENT "Generating EDC classes for database ${ARG_DATABASE_NAME} into ${ARG_OUTPUT_FOLDER}..."
        OUTPUT ${EDC_HEADER_FILE}
        OUTPUT ${EDC_CPP_FILE}
        COMMAND ${GAIA_GAIAC_CMD}
        -o ${ARG_OUTPUT_FOLDER}
        -d ${ARG_DATABASE_NAME}
        ${ARG_INSTANCE_NAME}
        -g
        DEPENDS ${ARG_DDL_FILE}
    )
  endif()

  if(NOT DEFINED ARG_TARGET_NAME)
    set(ARG_TARGET_NAME "generate_edc_${DDL_NAME}")
    message(STATUS "TARGET_NAME not provided, using default value: ${ARG_TARGET_NAME}.")
  endif()

  add_custom_target(${ARG_TARGET_NAME} ALL
      DEPENDS ${EDC_HEADER_FILE}
      DEPENDS ${EDC_CPP_FILE}
  )

  set(GAIA_EDC_GENERATION_TARGETS "${GAIA_EDC_GENERATION_TARGETS};${ARG_TARGET_NAME}" PARENT_SCOPE)
  set(GAIA_EDC_GENERATED_HEADERS "${GAIA_EDC_GENERATED_HEADERS};${EDC_HEADER_FILE}" PARENT_SCOPE)
  set(GAIA_EDC_GENERATED_CPP "${GAIA_EDC_GENERATED_CPP};${EDC_CPP_FILE}" PARENT_SCOPE)
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
# - OUTPUT_FOLDER: [optional] folder where the header files will be generated.
#     If not provided the default value is ${GAIA_DEFAULT_EDC_GENERATED_DIR}/${RULESET_NAME}
# - TARGET_NAME: [optional] the name of the generated target.
#     If not provided the default value is translate_${RULESET_NAME}_ruleset.
# - CLANG_PARAMS: [optional]: Additional parameters to pass to clang (invoked by gaiat)
# - INSTANCE_NAME: [optional] name of the database instance gaiat will connect to.
# - DEPENDS: [optional] optional list of targets this task depends on.
#            Typically the translation has to depend on the generation of the
#            schema headers.
#            If not provided will depend on the targets listed in ${GAIA_EDC_GENERATION_TARGETS}
function(translate_ruleset)
  set(options "")
  set(oneValueArgs RULESET_FILE OUTPUT_FOLDER TARGET_NAME GAIAT_CMD)
  set(multiValueArgs CLANG_PARAMS DEPENDS)
  cmake_parse_arguments("ARG" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  check_param(ARG_RULESET_FILE)

  get_filename_component(RULESET_NAME ${ARG_RULESET_FILE} NAME)
  string(REPLACE ".ruleset" "" RULESET_NAME ${RULESET_NAME})

  if(NOT DEFINED ARG_OUTPUT_FOLDER)
    set(ARG_OUTPUT_FOLDER ${GAIA_DEFAULT_RULES_GENERATED_DIR}/${RULESET_NAME})
    message(STATUS "OUTPUT_FOLDER not provided, using: ${ARG_OUTPUT_FOLDER}.")
    file(MAKE_DIRECTORY ${ARG_OUTPUT_FOLDER})
  endif()

  set(RULESET_CPP_NAME ${RULESET_NAME}_ruleset.cpp)
  set(RULESET_CPP_PATH ${ARG_OUTPUT_FOLDER}/${RULESET_CPP_NAME})

  message(STATUS "Adding target for translating ruleset: ${ARG_RULESET_FILE} into ${RULESET_CPP_NAME}.")

  set(GAIAT_INCLUDE_PATH "")

  # Add implicit include directories
  foreach(INCLUDE_PATH ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
    # Have to use ; instead of space otherwise custom_command will try to escape it
    string(APPEND GAIAT_INCLUDE_PATH "-I;${INCLUDE_PATH};")
  endforeach()

  # Add default Gaia path
  string(APPEND GAIAT_INCLUDE_PATH "-I;/opt/gaia/include;")

  # Add the output folder (which contains the DDL headers)

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
    message(STATUS "TARGET_NAME not provided, using default value: ${ARG_TARGET_NAME}.")
  endif()

  add_custom_target(${ARG_TARGET_NAME} ALL
    DEPENDS ${RULESET_CPP_PATH})

  set(GAIA_RULES_TRANSLATION_TARGETS "${GAIA_RULES_TRANSLATION_TARGETS};${ARG_TARGET_NAME}" PARENT_SCOPE)
  set(GAIA_RULES_TRANSLATED_CPP "${GAIA_RULES_TRANSLATED_CPP};${RULESET_CPP_PATH}" PARENT_SCOPE)
endfunction()


#
# TARGET_NAME
function(target_add_gaia_generated_sources)
  set(options "")
  set(oneValueArgs TARGET_NAME)
  set(multiValueArgs "")
  cmake_parse_arguments("ARG" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  check_param(ARG_TARGET_NAME)

  foreach(TARGET ${GAIA_EDC_GENERATION_TARGETS})
    message(STATUS "Adding ${TARGET} to ${ARG_TARGET_NAME}...")
    add_dependencies(${ARG_TARGET_NAME} ${TARGET})
  endforeach()

  foreach(TARGET ${GAIA_RULES_TRANSLATION_TARGETS})
    message(STATUS "Adding ${TARGET} to ${ARG_TARGET_NAME}...")
    add_dependencies(${ARG_TARGET_NAME} ${TARGET})
  endforeach()

  foreach(CPP_FILE ${GAIA_EDC_GENERATED_CPP})
    message(STATUS "Adding ${CPP_FILE} to ${ARG_TARGET_NAME}...")
    target_sources(${ARG_TARGET_NAME} PRIVATE ${CPP_FILE})
  endforeach()

  foreach(HEADER_FILE ${GAIA_EDC_GENERATED_HEADERS})
    get_filename_component(HEADER_DIR ${HEADER_FILE} DIRECTORY)
    message(STATUS "Adding ${HEADER_DIR} to ${ARG_TARGET_NAME}...")
    target_include_directories(${ARG_TARGET_NAME} PUBLIC ${HEADER_DIR})
  endforeach()

  foreach(CPP_FILE ${GAIA_RULES_TRANSLATED_CPP})
    message(STATUS "Adding ${CPP_FILE} to ${ARG_TARGET_NAME}...")
    target_sources(${ARG_TARGET_NAME} PRIVATE ${CPP_FILE})
  endforeach()

endfunction()
