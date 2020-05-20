#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)

# Set the project name.
project(cow_se)

# C++ unit test.
add_executable(test_cow_se tests/test_cow_se.cpp)
target_include_directories(test_cow_se PRIVATE ${PROJECT_SOURCE_DIR})
target_include_directories(test_cow_se PRIVATE inc/)
target_link_libraries(test_cow_se PRIVATE rt)
set_target_properties(test_cow_se PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -Wno-unused-parameter -Wno-sign-compare")
set_target_properties(test_cow_se PROPERTIES LINK_FLAGS ${GAIA_LINK_FLAGS})
set_test(test_cow_se 0 ${TEST_SUCCESS})

# The PyBind11 wrapper library.
# This requires python 2.7 because it has to work with multicorn.
set(PYBIND11_PYTHON_VERSION 2.7 CACHE STRING "")
find_package(Python2 COMPONENTS Interpreter Development)
find_package(pybind11)
if (Python2_FOUND AND pybind11_FOUND)
  pybind11_add_module(cow_se src/cow_se_pywrap.cpp)
  target_include_directories(cow_se PRIVATE ${pybind11_INCLUDE_DIR})
  target_include_directories(cow_se PRIVATE inc/)
  target_link_libraries(cow_se PRIVATE rt)
  set_target_properties(cow_se PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -Wno-deprecated-register -Wno-unused-parameter")
  set_target_properties(cow_se PROPERTIES LINK_FLAGS ${GAIA_LINK_FLAGS})

  # Copy file required for packaging out the Python wrapper library.
  configure_file(src/__init__.py . COPYONLY)

  # Python wrapper unit test.
  configure_file(tests/test_cow_se.py . COPYONLY)
  add_test(NAME test_cow_se_py COMMAND ${Python2_EXECUTABLE} test_cow_se.py)
  set_tests_properties(test_cow_se_py PROPERTIES PASS_REGULAR_EXPRESSION ${TEST_SUCCESS})
endif()
