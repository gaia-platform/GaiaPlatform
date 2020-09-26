#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.17)

include(UseJava)

enable_testing()

# Set the project name.
project(production)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

include(gaia)
include(GoogleTest)

# Specify the C++ standard.
set(CMAKE_CXX_STANDARD 17)
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
set(ROCKSDB "${GAIA_REPO}/third_party/production/rocksdb")
set(ROCKSDB_INC "${ROCKSDB}/include")
set(GREMLIN_CONSOLE_PATH "/usr/local/share/gremlin-console")
set(GREMLIN_SERVER_PATH "/usr/local/share/gremlin-server")
set(GAIA_PROD_BUILD "${CMAKE_CURRENT_BINARY_DIR}")
set(GAIA_PARSER_GENERATED "${GAIA_PROD_BUILD}/catalog/parser/generated")
set(GAIA_GENERATED_SCHEMAS "${GAIA_PROD_BUILD}/schemas/generated")

message(VERBOSE "GAIA_INC=${GAIA_INC}")

message(CHECK_START "Looking for Python3")
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
if(Python3_FOUND)
  list(APPEND CMAKE_MESSAGE_INDENT "  ")
  message(VERBOSE "Python3_EXECUTABLE=${Python3_EXECUTABLE}")
  message(VERBOSE "Python3_INCLUDE_DIRS=${Python3_INCLUDE_DIRS}")
  list(POP_BACK CMAKE_MESSAGE_INDENT)
  message(CHECK_PASS "found")
else()
  message(CHECK_FAIL "not found")
endif()

message(CHECK_START "Looking for Java")
find_package(Java)
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
if(JAVA_FOUND)
  list(APPEND CMAKE_MESSAGE_INDENT "  ")
  message (VERBOSE "Java_JAVAC_EXECUTABLE=${Java_JAVAC_EXECUTABLE}")
  message (VERBOSE "Java_JAVA_EXECUTABLE=${Java_JAVA_EXECUTABLE}")
  list(POP_BACK CMAKE_MESSAGE_INDENT)
  message(CHECK_PASS "found")
else()
  message(CHECK_FAIL "not found")
endif()

message(CHECK_START "Looking for JNI")
find_package(JNI)
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
if(JNI_FOUND)
  list(APPEND CMAKE_MESSAGE_INDENT "  ")
  message (VERBOSE "JNI_INCLUDE_DIRS=${JNI_INCLUDE_DIRS}")
  message (VERBOSE "JNI_LIBRARIES=${JNI_LIBRARIES}")
  list(POP_BACK CMAKE_MESSAGE_INDENT)
  message(CHECK_PASS "found")
else()
  message(CHECK_FAIL "not found")
endif()

# Java Tinkerpop project.
if(JAVA_FOUND AND JNI_FOUND AND EXISTS "${GREMLIN_CONSOLE_PATH}")
  set(CMAKE_JAVA_INCLUDE_PATH ${GREMLIN_CONSOLE_PATH}/lib/*:${GREMLIN_CONSOLE_PATH}/ext/tinkergraph-gremlin/lib/*:${PROJECT_BINARY_DIR}/*)

  # Java Tinkerpop graph provider implementation. This also generates native headers for the COW wrapper.
  add_jar(GaiaTinkerpop
    SOURCES
      java/com/gaiaplatform/database/CowStorageEngine.java
      java/com/gaiaplatform/database/Airline.java
      java/com/gaiaplatform/database/Airport.java
      java/com/gaiaplatform/database/Route.java
      java/com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheEdge.java
      java/com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheElement.java
      java/com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheFactory.java
      java/com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheGraph.java
      java/com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheHelper.java
      java/com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheProperty.java
      java/com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheTransaction.java
      java/com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheVertex.java
      java/com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheVertexProperty.java
      ${FLATBUFFERS}/java/com/google/flatbuffers/ArrayReadWriteBuf.java
      ${FLATBUFFERS}/java/com/google/flatbuffers/BaseVector.java
      ${FLATBUFFERS}/java/com/google/flatbuffers/BooleanVector.java
      ${FLATBUFFERS}/java/com/google/flatbuffers/ByteBufferReadWriteBuf.java
      ${FLATBUFFERS}/java/com/google/flatbuffers/ByteBufferUtil.java
      ${FLATBUFFERS}/java/com/google/flatbuffers/ByteVector.java
      ${FLATBUFFERS}/java/com/google/flatbuffers/Constants.java
      ${FLATBUFFERS}/java/com/google/flatbuffers/DoubleVector.java
      ${FLATBUFFERS}/java/com/google/flatbuffers/FlatBufferBuilder.java
      ${FLATBUFFERS}/java/com/google/flatbuffers/FlexBuffers.java
      ${FLATBUFFERS}/java/com/google/flatbuffers/FlexBuffersBuilder.java
      ${FLATBUFFERS}/java/com/google/flatbuffers/FloatVector.java
      ${FLATBUFFERS}/java/com/google/flatbuffers/IntVector.java
      ${FLATBUFFERS}/java/com/google/flatbuffers/LongVector.java
      ${FLATBUFFERS}/java/com/google/flatbuffers/ReadBuf.java
      ${FLATBUFFERS}/java/com/google/flatbuffers/ReadWriteBuf.java
      ${FLATBUFFERS}/java/com/google/flatbuffers/ShortVector.java
      ${FLATBUFFERS}/java/com/google/flatbuffers/StringVector.java
      ${FLATBUFFERS}/java/com/google/flatbuffers/Struct.java
      ${FLATBUFFERS}/java/com/google/flatbuffers/Table.java
      ${FLATBUFFERS}/java/com/google/flatbuffers/UnionVector.java
      ${FLATBUFFERS}/java/com/google/flatbuffers/Utf8.java
      ${FLATBUFFERS}/java/com/google/flatbuffers/Utf8Old.java
      ${FLATBUFFERS}/java/com/google/flatbuffers/Utf8Safe.java
    GENERATE_NATIVE_HEADERS storage_engine_jni_headers
      DESTINATION ${PROJECT_BINARY_DIR}/db/storage_engine/)
  get_target_property(GAIA_TINKERPOP_JAR GaiaTinkerpop JAR_FILE)

  # Java tests.
  add_jar(GaiaTests
    SOURCES
      java/com/gaiaplatform/tests/database/cachegraph/TestCacheGraph.java
    INCLUDE_JARS GaiaTinkerpop)
  get_target_property(GAIA_TESTS_JAR GaiaTests JAR_FILE)

  # FIXME: fix these tests in https://gaiaplatform.atlassian.net/browse/GAIAPLAT-260.
  # Add JNI test.
  # add_test(
  #   NAME test_se_jni
  #   COMMAND ${Java_JAVA_EXECUTABLE}
  #     -Djava.library.path=${PROJECT_BINARY_DIR}/db/storage_engine
  #     -cp ${GAIA_TINKERPOP_JAR}
  #     com.gaiaplatform.database.CowStorageEngine)

  # Add CacheGraph tests.
#   add_test(
#     NAME test_cachegraph_basic
#     COMMAND ${Java_JAVA_EXECUTABLE} -ea
#       -Djava.library.path=${PROJECT_BINARY_DIR}/db/storage_engine
#       -cp ${CMAKE_JAVA_INCLUDE_PATH}:${GAIA_TINKERPOP_JAR}:${GAIA_TESTS_JAR}
#       com.gaiaplatform.tests.database.cachegraph.TestCacheGraph basic)
#   add_test(
#     NAME test_cachegraph_factory
#     COMMAND ${Java_JAVA_EXECUTABLE} -ea
#       -Djava.library.path=${PROJECT_BINARY_DIR}/db/storage_engine
#       -cp ${CMAKE_JAVA_INCLUDE_PATH}:${GAIA_TINKERPOP_JAR}:${GAIA_TESTS_JAR}
#       com.gaiaplatform.tests.database.cachegraph.TestCacheGraph factory)
#   add_test(
#     NAME test_cachegraph_graphml
#     COMMAND ${Java_JAVA_EXECUTABLE} -ea
#       -Djava.library.path=${PROJECT_BINARY_DIR}/db/storage_engine
#       -cp ${CMAKE_JAVA_INCLUDE_PATH}:${GAIA_TINKERPOP_JAR}:${GAIA_TESTS_JAR}
#       com.gaiaplatform.tests.database.cachegraph.TestCacheGraph graphml)
endif()

# Add rocksdb
# https://gaiaplatform.atlassian.net/browse/GAIAPLAT-338
add_subdirectory(${ROCKSDB} rocksdb)

# Add flatbuffers
add_subdirectory(${FLATBUFFERS} flatbuffers)
set_property (TARGET flatbuffers PROPERTY POSITION_INDEPENDENT_CODE ON)

# Add individual component folders.
add_subdirectory(common)
add_subdirectory(db)
add_subdirectory(direct_access)
add_subdirectory(rules)
add_subdirectory(schemas)
add_subdirectory(sql)
add_subdirectory(system)
add_subdirectory(catalog)
add_subdirectory(tools)

# Add googletest
add_subdirectory(${GOOGLE_TEST} googletest)

# Customize test commands.
file(GENERATE OUTPUT "${PROJECT_BINARY_DIR}/CTestCustom.cmake"
  CONTENT "set(CTEST_CUSTOM_PRE_TEST \"daemonize $<TARGET_FILE:gaia_se_server>\")\nset(CTEST_CUSTOM_POST_TEST \"pkill -f -KILL gaia_se_server\")\n"
)
