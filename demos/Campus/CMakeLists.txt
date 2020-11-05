#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)
enable_testing()

# Set the project name.
project(campus_demo)

find_package(GTest REQUIRED)
#find_package(GMock REQUIRED)

include(GoogleTest)

set(GAIA_COMPILE_FLAGS "-c -Wall -Wextra -ggdb")
set(GENERATED_OUTPUTS "${CMAKE_BINARY_DIR}/campus_demo/generated")

#set(GOOGLE_TEST "${GAIA_REPO}/third_party/production/googletest")
#set(GOOGLE_TEST_INC "${GOOGLE_TEST}/googletest/include")

# Googletest
#add_subdirectory(${GOOGLE_TEST} googletest)

set(GAIA_CAMPUS_DEMO_INCLUDES
  ${GTEST_INCLUDE_DIRS}
  #${GMOCK_INCLUDE_DIRS}
  ${GAIA_INC}/public/common
  ${GAIA_INC}/public/direct_access
  ${GAIA_INC}/public/system
  ${GAIA_INC}/internal/common
  ${GAIA_INC}/public/rules
  ${GAIA_INC}/internal/rules
  ${GAIA_INC}/public/db
  ${GAIA_INC}/internal/db
  ${FLATBUFFERS_INC}
  ${GENERATED_OUTPUTS}
)

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
    tests/test_campus_demo.cpp
#!    "${GENERATED_OUTPUTS}/rules_init.cpp"
)

#!add_dependencies(campus generate_barn_storage)
set_target_properties(campus PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")  
target_include_directories(campus PRIVATE ${GAIA_CAMPUS_DEMO_INCLUDES})
#1target_link_libraries(campus PRIVATE rt pthread gaia gaia_semock gaia_rules gaia_direct)
target_link_libraries(campus PRIVATE ${GTEST_LIBRARIES} rt pthread )
