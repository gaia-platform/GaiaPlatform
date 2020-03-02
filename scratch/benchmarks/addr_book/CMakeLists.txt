#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.10)
project(addr_book)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED_ON)

include_directories(../../../external/flatbuffers/include ../../../demos/cow_se/inc ../inc ../perftimer)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/addr_book_generated.h"
    COMMAND flatc --cpp --gen-object-api  -o ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/addr_book.fbs
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/addr_book.fbs
    COMMENT "Compiling addr_book.fbs"
)

add_custom_target(whatever ALL DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/addr_book_generated.h")

add_executable(addr_book addr_book.cpp)
add_dependencies(addr_book whatever)

set(GAIA_COMPILE_FLAGS "-c -ggdb")
set(GAIA_LINK_FLAGS "-ggdb")

target_link_libraries(addr_book PRIVATE rt uuid)

set_target_properties(addr_book PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(addr_book PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
