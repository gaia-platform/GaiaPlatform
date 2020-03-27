#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)

# Set the project name.
project(storage_engine_mock)

# JNI wrapper for storage engine mock.
if (JAVA_FOUND AND JNI_FOUND AND EXISTS "${GREMLIN_CONSOLE_PATH}")
  # Configure build of native C++ wrapper library.
  add_library(jni_se_mock SHARED storage_engine_jni_wrapper.cpp)
  target_include_directories(jni_se_mock PRIVATE ${PROJECT_SOURCE_DIR})
  target_include_directories(jni_se_mock PRIVATE ${PROJECT_BINARY_DIR})
  target_include_directories(jni_se_mock PRIVATE ${JNI_INCLUDE_DIRS})
  target_link_libraries(jni_se_mock PRIVATE
    rt
    storage_engine_mock_jni_headers)
  set_target_properties(jni_se_mock PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -Wno-deprecated-register -Wno-unused-parameter")
  set_target_properties(jni_se_mock PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
endif()
