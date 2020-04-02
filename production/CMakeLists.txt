#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)

include(UseJava)

enable_testing()

# Set the project name.
project(production)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

include(gaia)
include(GoogleTest)

# Specify the C++ standard.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set some global variables.
get_repo_root(${PROJECT_SOURCE_DIR} GAIA_REPO)
set(GAIA_COMPILE_FLAGS "-c -Wall -Wextra -ggdb")
set(GAIA_LINK_FLAGS "-ggdb -pthread")
set(GAIA_INC "${PROJECT_SOURCE_DIR}/inc")
set(GOOGLE_TEST "${GAIA_REPO}/third_party/production/googletest")
set(GOOGLE_TEST_INC "${GOOGLE_TEST}/googletest/include")
set(FLATBUFFERS "${GAIA_REPO}/third_party/production/flatbuffers")
set(FLATBUFFERS_INC "${FLATBUFFERS}/include")
set(PYBIND_PATH "${GAIA_REPO}/third_party/production/pybind11")
set(GREMLIN_CONSOLE_PATH "/usr/local/share/gremlin-console")
set(GREMLIN_SERVER_PATH "/usr/local/share/gremlin-server")

message(STATUS "GAIA_INC=${GAIA_INC}")

find_program(PYTHON "python")

find_package(Java)
find_package(JNI)

if (JAVA_FOUND)
  message (STATUS "Java_JAVAC_EXECUTABLE=${Java_JAVAC_EXECUTABLE}")
  message (STATUS "Java_JAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}")
endif()

if (JNI_FOUND)
  message (STATUS "JNI_INCLUDE_DIRS=${JNI_INCLUDE_DIRS}")
  message (STATUS "JNI_LIBRARIES=${JNI_LIBRARIES}")
endif()

# Java Tinkerpop project.
if (JAVA_FOUND AND JNI_FOUND AND EXISTS "${GREMLIN_CONSOLE_PATH}")
  set(CMAKE_JAVA_INCLUDE_PATH ${GREMLIN_CONSOLE_PATH}/lib/*:${GREMLIN_CONSOLE_PATH}/ext/tinkergraph-gremlin/lib/*)

  # Java wrapper code. This generates native headers for the native wrapper.
  add_jar(GaiaTinkerpop
    SOURCES
      java/com/gaiaplatform/database/CowStorageEngine.java
      java/com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheEdge.java
      java/com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheElement.java
      java/com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheFactory.java
      java/com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheGraph.java
      java/com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheHelper.java
      java/com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheProperty.java
      java/com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheVertex.java
      java/com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheVertexProperty.java
    GENERATE_NATIVE_HEADERS storage_engine_mock_jni_headers
    DESTINATION ${PROJECT_BINARY_DIR}/db/storage_engine/mock)
  get_target_property(GAIA_TINKERPOP_JAR GaiaTinkerpop JAR_FILE)

  # Add a test that executes the Java program.
  add_test(
    NAME test_se_mock_jni
    COMMAND ${Java_JAVA_EXECUTABLE}
      -Djava.library.path=./db/storage_engine
      -cp ${GAIA_TINKERPOP_JAR} com.gaiaplatform.database.CowStorageEngine)
endif()

# Common code - currently, a headers-only library.
add_library(common INTERFACE)
target_sources(common INTERFACE 
  ${PROJECT_SOURCE_DIR}/inc/internal/common/gaia_exception.hpp 
  ${PROJECT_SOURCE_DIR}/inc/internal/common/retail_assert.hpp
)

# Add flatbuffers
add_subdirectory("../third_party/production/flatbuffers" flatbuffers)

# Add individual component folders.
add_subdirectory(system)
add_subdirectory(db)
add_subdirectory(rules)
add_subdirectory(tests)

# Add googletest
add_subdirectory(${GOOGLE_TEST} googletest)
