#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)

# Set the project name.
project(expr)

set(GAIA_EXPR_INCLUDES
  "${GAIA_INC}/public/common"
  "${GAIA_INC}/public/expr")

message(STATUS "GAIA_EXPR_INCLUDES=${GAIA_EXPR_INCLUDES}")

# Our main target.
# add_library(gaia_expr SHARED
#    src/events.cpp)
#target_include_directories(gaia_expr PUBLIC ${GAIA_TRIGGERS_INCLUDES})
#set_target_properties(gaia_expr PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
#target_link_libraries(gaia_expr PRIVATE "gaia_common")

# Tests.
add_gtest(test_expr tests/test_expr.cpp "${GAIA_EXPR_INCLUDES}" "gaia_common;flatbuffers")
