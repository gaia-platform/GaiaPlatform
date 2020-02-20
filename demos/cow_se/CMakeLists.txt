#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.10)

# Set the project name.
project(cow_se)

# Specify pybind11 location.
set(pybind11_DIR ../third_party/production/pybind11)

# C++ unit test.
add_executable(test_cow_se tests/test_cow_se.cpp)
target_include_directories(test_cow_se PRIVATE ${PROJECT_SOURCE_DIR})
target_include_directories(test_cow_se PRIVATE inc/)
target_link_libraries(test_cow_se PRIVATE rt)
set_target_properties(test_cow_se PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -Wno-unused-parameter -Wno-sign-compare")
set_target_properties(test_cow_se PROPERTIES LINK_FLAGS ${GAIA_LINK_FLAGS})
set_test(test_cow_se 0 ${TEST_SUCCESS})

# The PyBind11 wrapper library.
find_package(PythonInterp)
find_package(pybind11)
pybind11_add_module(cow_se src/cow_se_pywrap.cpp)
target_include_directories(cow_se PRIVATE ../../third_party/production/pybind11)
target_include_directories(cow_se PRIVATE inc/)
target_link_libraries(cow_se PRIVATE rt)
set_target_properties(cow_se PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -Wno-deprecated-register -Wno-unused-parameter")
set_target_properties(cow_se PROPERTIES LINK_FLAGS ${GAIA_LINK_FLAGS})

# Copy file required for packaging out the Python wrapper library.
configure_file(src/__init__.py . COPYONLY)

# Python wrapper unit test.
configure_file(tests/test_cow_se.py . COPYONLY)
#set_test(${PYTHON_EXECUTABLE} test_cow_se.py ${TEST_SUCCESS})
add_test(NAME test_cow_se_py COMMAND ${PYTHON_EXECUTABLE} test_cow_se.py)
set_tests_properties(test_cow_se_py PROPERTIES PASS_REGULAR_EXPRESSION ${TEST_SUCCESS})


