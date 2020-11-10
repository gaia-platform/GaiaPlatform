#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

# Set the project name.
project(campus_demo)

find_package(Curses REQUIRED)
find_package(GTest REQUIRED)
find_package(spdlog REQUIRED)
#find_package(gaia REQUIRED)
#find_package(GMock REQUIRED)

set(GAIA_INC "/usr/local/include/gaia") #TODO: remove fixed path

find_library(MENU_LIBRARY menu)

include(GoogleTest)

set(GAIA_COMPILE_FLAGS "-c -Wall -Wextra -ggdb")
#!set(GENERATED_OUTPUTS "${CMAKE_BINARY_DIR}/campus_demo/generated")
set(GENERATED_OUTPUTS "${PROJECT_SOURCE_DIR}/generated")

set(GAIA_CAMPUS_DEMO_INCLUDES
  ${GTEST_INCLUDE_DIRS}
  ${CURSES_INCLUDE_DIR}
  #${GMOCK_INCLUDE_DIRS}
  ${GAIA_INC}/catalog
  ${GAIA_INC}/common
  ${GAIA_INC}/db
  ${GAIA_INC}/direct_access
  ${GAIA_INC}/rules
  ${GAIA_INC}/system
  ${FLATBUFFERS_INC}
  ${GENERATED_OUTPUTS}
)

# --- Generate ---
set(GAIA_GENERATE /usr/local/bin/gaiac) #TODO: remove fixed path

set(INCUBATOR_DDL ${PROJECT_SOURCE_DIR}/src/incubator.ddl)
set(INCUBATOR_RULESET ${PROJECT_SOURCE_DIR}/incubator.ruleset)
file(MAKE_DIRECTORY ${GENERATED_OUTPUTS})

add_custom_command(
  OUTPUT ${GENERATED_OUTPUTS}/gaia_incubator.h
  COMMAND ${GAIA_GENERATE} -o ${GENERATED_OUTPUTS} -g ${INCUBATOR_DDL}
  DEPENDS ${INCUBATOR_DDL}
  COMMENT "Compiled incubator.ddl."
)

# 20201108 disable for now because se_server throws illegal instruction
#add_custom_target(generate_incubator_headers ALL
#  DEPENDS ${GENERATED_OUTPUTS}/gaia_incubator.h
#)

#--- Translate ---
#set(GAIA_TRANSLATE "/home/dax/ASTGenerator/ASTCodeGenerator")

add_custom_target(make_generated_campus_dir ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_OUTPUTS}
)

set_property(GLOBAL PROPERTY FBS_GENERATED_OUTPUTS)
#!gaia_compile_flatbuffers_schema_to_cpp_opt(barn_storage.fbs "--no-includes;--gen-compare;--gaia-type-initial-value;1000" "${GENERATED_OUTPUTS}")
#!gaia_get_generated_output(fbs_generated)
if(fbs_generated)
  # message(STATUS "Add generated_code target with files:${fbs_generated}")
#!  add_custom_target(generate_barn_storage
#!    DEPENDS ${fbs_generated}
#!    COMMENT "All generated files were updated.")
endif()

add_library(gaia SHARED IMPORTED GLOBAL)
set_target_properties(gaia PROPERTIES IMPORTED_LOCATION /usr/local/lib/libgaia.so) #TODO: remove fixed path

add_library(gaia_index SHARED IMPORTED GLOBAL)
set_target_properties(gaia_index PROPERTIES IMPORTED_LOCATION /usr/local/lib/libgaia_index.so) #TODO: remove fixed path

add_library(gaia_storage SHARED IMPORTED GLOBAL)
set_target_properties(gaia_storage PROPERTIES IMPORTED_LOCATION /usr/local/lib/libgaia_storage.so) #TODO: remove fixed path

add_library(gaia_triggers SHARED IMPORTED GLOBAL)
set_target_properties(gaia_triggers PROPERTIES IMPORTED_LOCATION /usr/local/lib/libgaia_triggers.so) #TODO: remove fixed path

add_library(gaia_memorymanager SHARED IMPORTED GLOBAL)
set_target_properties(gaia_memorymanager PROPERTIES IMPORTED_LOCATION /usr/local/lib/libgaia_memorymanager.so) #TODO: remove fixed path

#!add_library(gaia SHARED IMPORTED GLOBAL)
#!set_target_properties(gaia PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/system/libgaia.a)

#!add_library(gaia_semock SHARED IMPORTED GLOBAL)
#!set_target_properties(gaia_semock PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/db/storage_engine/libgaia_semock.a)

#!add_library(gaia_rules SHARED IMPORTED GLOBAL)
#!set_target_properties(gaia_rules PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/rules/event_manager/libgaia_rules.a)

#!add_library(gaia_direct SHARED IMPORTED GLOBAL)
#!set_target_properties(gaia_direct PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/direct_access/libgaia_direct.a)

add_executable(campus 
    src/main.cpp
    src/rules.cpp
    src/campus_demo.cpp
    src/campus_demo_term.cpp
    tests/test_campus_demo.cpp
#!    "${GENERATED_OUTPUTS}/rules_init.cpp"
)

#!add_dependencies(campus generate_barn_storage)
set_target_properties(campus PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")  
target_include_directories(campus PRIVATE ${GAIA_CAMPUS_DEMO_INCLUDES})
#1target_link_libraries(campus PRIVATE rt pthread gaia gaia_semock gaia_rules gaia_direct)

#set(SPDLOG_BUILD_SHARED OFF)
target_link_libraries(campus PUBLIC spdlog::spdlog) 

# 20201108 disable for now because se_server throws illegal instruction
#target_link_libraries(campus PRIVATE ${GTEST_LIBRARIES} ${MENU_LIBRARY} ${CURSES_LIBRARIES} rt pthread gaia gaia_index gaia_storage gaia_triggers gaia_memorymanager)
target_link_libraries(campus PRIVATE ${GTEST_LIBRARIES} ${MENU_LIBRARY} ${CURSES_LIBRARIES} rt pthread)
