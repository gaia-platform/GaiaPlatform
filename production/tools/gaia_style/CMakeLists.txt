###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

project(gaia_style)

set(GAIA_STYLE_INCLUDES
  ${GAIA_INC}
  ${PROJECT_SOURCE_DIR}/inc
  ${FLATBUFFERS_INC}
)

message(VERBOSE "GAIA_STYLE_INCLUDES=${GAIA_STYLE_INCLUDES}")

set(CMAKE_CXX_CLANG_TIDY clang-tidy)

add_executable(gaia_format tests/clang_format.cpp)

configure_gaia_target(gaia_format)
target_include_directories(gaia_format PRIVATE ${GAIA_STYLE_INCLUDES})
target_link_libraries(gaia_format PRIVATE rt gaia_catalog gaia_parser)
target_compile_options(gaia_format PRIVATE -Wno-unused-variable -Wno-unused-parameter)

add_executable(gaia_tidy tests/clang_tidy.cpp)

configure_gaia_target(gaia_tidy)
target_include_directories(gaia_tidy PRIVATE ${GAIA_STYLE_INCLUDES})
target_link_libraries(gaia_tidy PRIVATE rt gaia_catalog gaia_parser)
target_compile_options(gaia_tidy PRIVATE -Wno-unused-variable -Wno-unused-parameter)
