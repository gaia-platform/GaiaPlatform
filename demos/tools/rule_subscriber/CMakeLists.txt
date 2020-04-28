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

set(GENERATED_OUTPUTS "${CMAKE_BINARY_DIR}/tools/rule_subscriber/generated")
add_custom_target(make_generated_dir ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_OUTPUTS}
)

add_custom_command(
  OUTPUT "${GENERATED_OUTPUTS}/init_rules.cpp"
  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/gen_rules.sh"
    ${CMAKE_CURRENT_SOURCE_DIR}/test_rules2.hpp
    ${GENERATED_OUTPUTS}/init_rules.cpp
    ${RULE_SUBSCRIBER_INCLUDES}
    DEPENDS rule_subscriber
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/test_rules2.hpp
    COMMENT "Run rule_subscriber: ${GENERATED_OUTPUTS}/init_rules.cpp"
)

link_directories(
  ${GAIA_PROD_BUILD}/system
  ${GAIA_PROD_BUILD}/lib
  ${GAIA_PROD_BUILD}/db/storage_engine
  ${GAIA_PROD_BUILD}/rules/event_manager
)

add_gtest(test_rule_subscriber 
  "test_rule_subscriber.cpp;${GENERATED_OUTPUTS}/init_rules.cpp;" 
  "${RULE_SUBSCRIBER_INCLUDES}" 
  "rt;gaia_rules;gaia;gaia_direct;gaia_semock"
)
