#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)

if (EXISTS /usr/lib/llvm-8/lib/libclangBasic.a)
    add_subdirectory(rule_subscriber)
else()
    message(STATUS "To build rule subscriber libclang-8-dev should be installed")
endif()


