#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(gaiac)

set(GAIA_CATALOG_TOOL_INCLUDES
  ${FLATBUFFERS_INC}
  ${GAIA_REPO}/production/catalog/inc
  ${GAIA_REPO}/production/catalog/parser/inc
  ${GAIA_REPO}/production/db/storage_engine/inc
  ${GAIA_REPO}/production/db/inc/memory_manager
  ${GAIA_INC}
  ${GAIA_INTERNAL_INC}/db
  ${GAIA_INTERNAL_INC}/common
  ${GAIA_PARSER_GENERATED}
  ${PROJECT_SOURCE_DIR}/inc
)

message(VERBOSE "GAIA_CATALOG_TOOL_INCLUDES=${GAIA_CATALOG_TOOL_INCLUDES}")

add_executable(gaiac
  src/main.cpp
  src/command.cpp
  ${FLATBUFFERS}/src/idl_gen_cpp.cpp
  ${FLATBUFFERS}/src/code_generators.cpp)
target_include_directories(gaiac PRIVATE ${GAIA_CATALOG_TOOL_INCLUDES})
add_dependencies(gaiac gaia_se_server)
target_link_libraries(gaiac PRIVATE rt gaia_common gaia_catalog gaia_parser flatbuffers tabulate)
set_target_properties(gaiac PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(gaiac PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")

install(TARGETS gaiac DESTINATION ${CMAKE_INSTALL_BINDIR})
