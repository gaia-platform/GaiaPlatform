#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../production/cmake;${CMAKE_MODULE_PATH}")
include(gaia)
include(GoogleTest)

get_repo_root(${PROJECT_SOURCE_DIR} GAIA_REPO)

cmake_minimum_required(VERSION 3.11)

set(LLVM_LIB_PATH "/usr/lib/llvm-8")

list(APPEND CMAKE_MODULE_PATH 
  ${LLVM_LIB_PATH}/lib/cmake/clang
  ${LLVM_LIB_PATH}/lib/cmake/llvm
 )

set(CMAKE_CXX_FLAGS
      "-std=c++0x -Wall -Wextra -ggdb -Wno-unused-parameter -fno-rtti -DLLVM_DISABLE_ABI_BREAKING_CHECKS_ENFORCING=1")
set(CMAKE_EXE_LINKER_FLAGS "-ggdb -pthread")

add_executable(rule_subscriber RuleSubscriber.cpp)

set(search_paths
  ${LLVM_LIB_PATH}/lib/cmake/clang
  ${LLVM_LIB_PATH}/lib/cmake/llvm
)


find_package(LLVM REQUIRED CONFIG
  PATHS ${search_paths}
  NO_DEFAULT_PATH
)

find_package(Clang REQUIRED CONFIG
  PATHS ${search_paths}
  NO_DEFAULT_PATH
)

include_directories(
  ..
  ${LLVM_LIB_PATH}/include
  ${LLVM_LIB_PATH}/lib
)

target_link_libraries(rule_subscriber
  PRIVATE
  clangTooling
)

set(RULE_SUBSCRIBER_INCLUDES 
  ${GAIA_REPO}/third_party/production/googletest/googletest/include
  ${GAIA_REPO}/third_party/production/flatbuffers/include
  ${GAIA_REPO}/production/inc/public/common
  ${GAIA_REPO}/production/inc/public/rules
  ${GAIA_REPO}/production/inc/internal/common
  ${GAIA_REPO}/production/db/storage_engine/mock
  ${GAIA_REPO}/production/tests/common/direct_access
)

link_directories(
  ${GAIA_PROD_BUILD}/system
  ${GAIA_PROD_BUILD}/lib
  ${GAIA_PROD_BUILD}/db/storage_engine
  ${GAIA_PROD_BUILD}/rules/event_manager

)
add_gtest(test_rule_subscriber "test_rule_subscriber.cpp;init_rules.cpp" "${RULE_SUBSCRIBER_INCLUDES}" "rt;gaia_rules;gaia;gaia_direct;gaia_semock")
