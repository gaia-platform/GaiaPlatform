#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.16)

# Set the project name.
project(ping_pong)

set(CMAKE_CXX_STANDARD 17)

set(PING_PONG_DDL ${PROJECT_SOURCE_DIR}/ping_pong.ddl)
set(PING_PONG_RULESET ${PROJECT_SOURCE_DIR}/ping_pong.ruleset)

option(BUILD_SDK "Build ping_pong against the Gaia SDK instead of production" OFF)

if(BUILD_SDK)
  include(CMakeLists-sdk.cmake)
else()
  include(CMakeLists-internal.cmake)
endif()
