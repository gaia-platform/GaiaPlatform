#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(gaia_style)

set(GAIA_GENERATE_INCLUDES
  ${GAIA_INC}
  ${FLATBUFFERS_INC}
)

message(VERBOSE "GAIA_GENERATE_INCLUDES=${GAIA_GENERATE_INCLUDES}")

set(CMAKE_CXX_CLANG_TIDY clang-tidy)

add_executable(gaia_format tests/clang_format.cpp)

target_include_directories(gaia_format PRIVATE ${GAIA_GENERATE_INCLUDES})
target_link_libraries(gaia_format PRIVATE rt gaia_catalog gaia_parser)
set_target_properties(gaia_format PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -Wno-unused-variable -Wno-unused-parameter")
set_target_properties(gaia_format PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")

add_executable(gaia_tidy tests/clang_tidy.cpp)

target_include_directories(gaia_tidy PRIVATE ${GAIA_GENERATE_INCLUDES})
target_link_libraries(gaia_tidy PRIVATE rt gaia_catalog gaia_parser)
set_target_properties(gaia_tidy PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -Wno-unused-variable -Wno-unused-parameter")
set_target_properties(gaia_tidy PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
