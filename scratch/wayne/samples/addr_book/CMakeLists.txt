#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)
project(addr_book)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED_ON)

include_directories(
    ../../../../production/inc/public/common
    ../../../../production/db/storage_engine/mock
)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/addr_book_generated.h"
    COMMAND ${FLATC_DIR}/flatc --cpp --gen-object-api --cpp-str-type gaia::common::nullable_string_t --cpp-str-flex-ctor -o ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/addr_book.fbs
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/addr_book.fbs
    COMMENT "Compiling addr_book.fbs"
)

add_custom_target(generated_files ALL DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/addr_book_generated.h")

add_executable(addr_book addr_book.cpp)
add_dependencies(addr_book generated_files)

set(GAIA_COMPILE_FLAGS "-c -g")
set(GAIA_LINK_FLAGS "-g")

target_link_libraries(addr_book PRIVATE rt uuid)

set_target_properties(addr_book PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(addr_book PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
