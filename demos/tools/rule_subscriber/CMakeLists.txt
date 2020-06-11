#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)

project(rule_subscriber)

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

set(GENERATED_OUTPUTS "${CMAKE_BINARY_DIR}/tools/rule_subscriber/generated")

set(RULE_SUBSCRIBER_INCLUDES 
  ${GAIA_REPO}/third_party/production/googletest/googletest/include
  ${GAIA_REPO}/third_party/production/flatbuffers/include
  ${GAIA_REPO}/production/inc/public/common
  ${GAIA_REPO}/production/inc/public/rules
  ${GAIA_REPO}/production/inc/internal/rules
  ${GAIA_REPO}/production/inc/internal/common
  ${GAIA_REPO}/production/db/storage_engine/mock
  ${GENERATED_OUTPUTS}
)

add_custom_target(make_generated_rule_subscriber_dir ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_OUTPUTS}
)

# generate headers for Address schema
# generate the headers from the flatbuffer
set_property(GLOBAL PROPERTY FBS_GENERATED_OUTPUTS)
gaia_compile_flatbuffers_schema_to_cpp_opt(addr_book.fbs "--no-includes;--gen-compare;--gen-events;--gaia-type-initial-value;3000" "${GENERATED_OUTPUTS}")
gaia_get_generated_output(fbs_generated)
if(fbs_generated)
  # message(STATUS "Add generated_code target with files:${fbs_generated}")
  add_custom_target(generate_addr_book
    DEPENDS ${fbs_generated}
    COMMENT "All generated files were updated.")
endif()

# create rules initialization routine for test_rule_subscriber
gaia_gen_rule_subscriptions("${CMAKE_CURRENT_SOURCE_DIR}/test_rules.hpp" "${GENERATED_OUTPUTS}/init_rules.cpp" "${RULE_SUBSCRIBER_INCLUDES}")
gaia_gen_rule_subscriptions("${CMAKE_CURRENT_SOURCE_DIR}/test_rules_no_tx.hpp" "${GENERATED_OUTPUTS}/init_rules_no_tx.cpp" "${RULE_SUBSCRIBER_INCLUDES}")

link_directories(
  ${GAIA_PROD_BUILD}/system
  ${GAIA_PROD_BUILD}/lib
  ${GAIA_PROD_BUILD}/db/storage_engine
  ${GAIA_PROD_BUILD}/rules/event_manager
)

add_gtest(test_rule_subscriber 
  "test_rule_subscriber.cpp;${GENERATED_OUTPUTS}/init_rules.cpp" 
  "${RULE_SUBSCRIBER_INCLUDES}" 
  "rt;gaia_rules;gaia;gaia_direct;gaia_semock"
  "generate_addr_book"
)

add_gtest(test_rule_subscriber_no_tx 
  "test_rule_subscriber_no_tx.cpp;${GENERATED_OUTPUTS}/init_rules_no_tx.cpp" 
  "${RULE_SUBSCRIBER_INCLUDES}" 
  "rt;gaia_rules;gaia;gaia_direct;gaia_semock"
  "generate_addr_book"
)