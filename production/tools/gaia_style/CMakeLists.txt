#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(gaia_style)

set(GAIA_STYLE_INCLUDES
  ${GAIA_INC}/public/catalog
  ${GAIA_INC}/public/common
  ${GAIA_INC}/public/db
  ${GAIA_INC}/public/direct_access
  ${GAIA_INC}/public/
  ${PROJECT_SOURCE_DIR}/inc
  ${FLATBUFFERS_INC}
)

message(VERBOSE "GAIA_STYLE_INCLUDES=${GAIA_STYLE_INCLUDES}")

set(CMAKE_CXX_CLANG_TIDY clang-tidy)

add_executable(gaia_format tests/clang_format.cpp)

target_include_directories(gaia_format PRIVATE ${GAIA_STYLE_INCLUDES})
target_link_libraries(gaia_format PRIVATE rt gaia_catalog gaia_parser)
set_target_properties(gaia_format PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -Wno-unused-variable -Wno-unused-parameter")
set_target_properties(gaia_format PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")

add_executable(gaia_tidy tests/clang_tidy.cpp)

target_include_directories(gaia_tidy PRIVATE ${GAIA_STYLE_INCLUDES})
target_link_libraries(gaia_tidy PRIVATE rt gaia_catalog gaia_parser)
set_target_properties(gaia_tidy PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -Wno-unused-variable -Wno-unused-parameter")
set_target_properties(gaia_tidy PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
