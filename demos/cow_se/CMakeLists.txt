#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)

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
add_test(NAME test_cow_se_py COMMAND ${PYTHON_EXECUTABLE} test_cow_se.py)
set_tests_properties(test_cow_se_py PROPERTIES PASS_REGULAR_EXPRESSION ${TEST_SUCCESS})

if (JAVA_FOUND AND JNI_FOUND AND EXISTS "${GREMLIN_CONSOLE_PATH}")
  set(CMAKE_JAVA_INCLUDE_PATH ${GREMLIN_CONSOLE_PATH}/lib/*:${GREMLIN_CONSOLE_PATH}/ext/tinkergraph-gremlin/lib/*)

  # Java wrapper code. This generates native headers for the native wrapper.
  add_jar(TrueGraphDB
    SOURCES
      src/java/com/gaiaplatform/truegraphdb/CowStorageEngine.java
      src/java/com/gaiaplatform/truegraphdb/tinkerpop/gremlin/structure/TrueGraphDBEdge.java
      src/java/com/gaiaplatform/truegraphdb/tinkerpop/gremlin/structure/TrueGraphDBElement.java
      src/java/com/gaiaplatform/truegraphdb/tinkerpop/gremlin/structure/TrueGraphDBFactory.java
      src/java/com/gaiaplatform/truegraphdb/tinkerpop/gremlin/structure/TrueGraphDBGraph.java
      src/java/com/gaiaplatform/truegraphdb/tinkerpop/gremlin/structure/TrueGraphDBHelper.java
      src/java/com/gaiaplatform/truegraphdb/tinkerpop/gremlin/structure/TrueGraphDBProperty.java
      src/java/com/gaiaplatform/truegraphdb/tinkerpop/gremlin/structure/TrueGraphDBVertex.java
      src/java/com/gaiaplatform/truegraphdb/tinkerpop/gremlin/structure/TrueGraphDBVertexProperty.java
    GENERATE_NATIVE_HEADERS cow_se_jni_headers
    DESTINATION ${PROJECT_BINARY_DIR})
  get_target_property(COW_SE_JAR TrueGraphDB JAR_FILE)

  # Configure build of native C++ wrapper library.
  add_library(native_cow_se SHARED src/cow_se_jwrap.cpp)
  target_include_directories(native_cow_se PRIVATE inc/)
  target_include_directories(native_cow_se PRIVATE ${PROJECT_BINARY_DIR})
  target_include_directories(native_cow_se PRIVATE ${JNI_INCLUDE_DIRS})
  target_link_libraries(native_cow_se PRIVATE
      rt
      cow_se_jni_headers)
  set_target_properties(native_cow_se PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -Wno-deprecated-register -Wno-unused-parameter")
  set_target_properties(native_cow_se PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")

  configure_file(src/dump_cow_se.py . COPYONLY)

  # Add a test that executes the Java program.
  add_test(NAME test_cow_se_jni COMMAND ${Java_JAVA_EXECUTABLE} -Djava.library.path=. -cp ${COW_SE_JAR} com.gaiaplatform.truegraphdb.CowStorageEngine)
endif()
