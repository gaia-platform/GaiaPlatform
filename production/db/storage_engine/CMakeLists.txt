#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)

# Set the project name.
project(storage_engine)

set(GAIA_STORAGE_ENGINE_INCLUDES "${GAIA_INC}/internal/common;${GAIA_DB_INC}/storage_engine")
message(STATUS "GAIA_STORAGE_ENGINE_INCLUDES=${GAIA_STORAGE_ENGINE_INCLUDES}")

###############################################
# Storage engine mock-related code begins here.
###############################################
add_library(gaia_semock STATIC mock/storage_engine.cpp)
target_include_directories(gaia_semock PRIVATE ${GAIA_STORAGE_ENGINE_INCLUDES})
set_target_properties(gaia_semock PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")

# Specify pybind11 location.
set(pybind11_DIR ${PYBIND_PATH})

# PyBind11 wrapper for storage engine mock.
find_package(PythonInterp)
find_package(pybind11)
pybind11_add_module(se_mock mock/storage_engine_pybind_wrapper.cpp)
target_include_directories(se_mock PRIVATE ${PYBIND_PATH})
target_include_directories(se_mock PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(se_mock PRIVATE rt gaia_semock)
set_target_properties(se_mock PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -Wno-deprecated-register -Wno-unused-parameter")
set_target_properties(se_mock PROPERTIES LINK_FLAGS ${GAIA_LINK_FLAGS})

# Python wrapper unit test.
configure_file(tests/test_storage_engine_mock.py . COPYONLY)
add_test(NAME test_se_mock_pybind COMMAND ${PYTHON_EXECUTABLE} test_storage_engine_mock.py)
set_tests_properties(test_se_mock_pybind PROPERTIES PASS_REGULAR_EXPRESSION ${TEST_SUCCESS})

# JNI wrapper for storage engine mock.
if (JAVA_FOUND AND JNI_FOUND AND EXISTS "${GREMLIN_CONSOLE_PATH}")
  # Configure build of native C++ wrapper library.
  add_library(jni_se_mock SHARED mock/storage_engine_jni_wrapper.cpp)
  target_include_directories(jni_se_mock PRIVATE ${PROJECT_SOURCE_DIR})
  target_include_directories(jni_se_mock PRIVATE ${PROJECT_BINARY_DIR})
  target_include_directories(jni_se_mock PRIVATE ${JNI_INCLUDE_DIRS})
  target_link_libraries(jni_se_mock PRIVATE
    rt
    gaia_semock
    storage_engine_mock_jni_headers)
  set_target_properties(jni_se_mock PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -Wno-deprecated-register -Wno-unused-parameter")
  set_target_properties(jni_se_mock PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
endif()
