#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

project(gaiac)

set(GAIA_CATALOG_TOOL_INCLUDES
  ${GAIA_REPO}/production/catalog/inc
  ${GAIA_REPO}/production/catalog/parser/inc
  ${GAIA_REPO}/production/db/inc/core
  ${GAIA_INC}
  ${GAIA_PARSER_GENERATED}
  ${PROJECT_SOURCE_DIR}/inc
)

message(VERBOSE "GAIA_CATALOG_TOOL_INCLUDES=${GAIA_CATALOG_TOOL_INCLUDES}")

# These files is downloaded with ExternalProject_Add in third_party/production/flatbuffers.
# CMake is not able to detect this dependency and fails at config time.
# The GENERATED property makes cmake to skip the source check at config time.
set_source_files_properties(${FLATBUFFERS_SRC}/src/idl_gen_cpp.cpp PROPERTIES GENERATED TRUE)
set_source_files_properties(${FLATBUFFERS_SRC}/src/code_generators.cpp PROPERTIES GENERATED TRUE)

add_executable(gaiac
  src/main.cpp
  src/command.cpp
  ${FLATBUFFERS_SRC}/src/idl_gen_cpp.cpp
  ${FLATBUFFERS_SRC}/src/code_generators.cpp)

configure_gaia_target(gaiac)
target_include_directories(gaiac PRIVATE ${GAIA_CATALOG_TOOL_INCLUDES})
target_link_libraries(gaiac PRIVATE rt gaia_common gaia_catalog gaia_parser flatbuffers tabulate Threads::Threads)

if(ENABLE_STACKTRACE)
  target_link_libraries(gaiac PRIVATE gaia_stack_trace)
endif()

install(TARGETS gaiac DESTINATION ${CMAKE_INSTALL_BINDIR})
