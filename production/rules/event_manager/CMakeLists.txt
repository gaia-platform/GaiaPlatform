#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.10)
enable_testing()

# Set the project name.
project(event_manager)

#list(APPEND CMAKE_MODULE_PATH "/home/dax/git/GaiaPlatform/production/cmake")
message(STATUS ${PROJECT_SOURCE_DIR})
#message(STATUS ${CMAKE_MODULE_PATH})

#find_package(GaiaPlatform REQUIRED)

#include(GaiaPlatform)


set(GAIA_EVENT_MANAGER_INCLUDES "../../inc/api;../inc;./inc;")
message(STATUS ${PROJECT_SOURCE_DIR})

add_library(event_manager SHARED
    src/event_manager.cpp
)

target_include_directories(event_manager PRIVATE ${GAIA_EVENT_MANAGER_INCLUDES})
    
#message(STATUS "GAIA_MEMORY_MANAGER_INCLUDES=${GAIA_MEMORY_MANAGER_INCLUDES}")

# Our unit tests.
#add_executable(test_event_manager tests/test_event_manager.cpp)
#target_include_directories(test_event_manager PRIVATE 
#    ${GAIA_EVENT_MANAGER_INCLUDES},
#    )
#target_link_libraries(test_event_manager PRIVATE event_manager)
#set_test(test_event_manager 0 ${TEST_SUCCESS})

