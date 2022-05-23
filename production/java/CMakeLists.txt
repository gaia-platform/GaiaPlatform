###################################################
# Copyright (c) Gaia Platform LLC
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

project(java)

include(UseJava)

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
if(JAVA_FOUND AND JNI_FOUND)
  # set(CMAKE_JAVA_INCLUDE_PATH ${GREMLIN_CONSOLE_PATH}/lib/*:${GREMLIN_CONSOLE_PATH}/ext/tinkergraph-gremlin/lib/*)

  # Java Tinkerpop graph provider implementation. This also generates native headers for the database wrapper.
  add_jar(GaiaTinkerpop
    SOURCES
    com/gaiaplatform/database/GaiaDatabase.java
    # com/gaiaplatform/database/Airline.java
    # com/gaiaplatform/database/Airport.java
    # com/gaiaplatform/database/Route.java
    # com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheEdge.java
    # com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheElement.java
    # com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheFactory.java
    # com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheGraph.java
    # com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheHelper.java
    # com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheProperty.java
    # com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheTransaction.java
    # com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheVertex.java
    # com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheVertexProperty.java
    # ${FLATBUFFERS}/com/google/flatbuffers/ArrayReadWriteBuf.java
    # ${FLATBUFFERS}/com/google/flatbuffers/BaseVector.java
    # ${FLATBUFFERS}/com/google/flatbuffers/BooleanVector.java
    # ${FLATBUFFERS}/com/google/flatbuffers/ByteBufferReadWriteBuf.java
    # ${FLATBUFFERS}/com/google/flatbuffers/ByteBufferUtil.java
    # ${FLATBUFFERS}/com/google/flatbuffers/ByteVector.java
    # ${FLATBUFFERS}/com/google/flatbuffers/Constants.java
    # ${FLATBUFFERS}/com/google/flatbuffers/DoubleVector.java
    # ${FLATBUFFERS}/com/google/flatbuffers/FlatBufferBuilder.java
    # ${FLATBUFFERS}/com/google/flatbuffers/FlexBuffers.java
    # ${FLATBUFFERS}/com/google/flatbuffers/FlexBuffersBuilder.java
    # ${FLATBUFFERS}/com/google/flatbuffers/FloatVector.java
    # ${FLATBUFFERS}/com/google/flatbuffers/IntVector.java
    # ${FLATBUFFERS}/com/google/flatbuffers/LongVector.java
    # ${FLATBUFFERS}/com/google/flatbuffers/ReadBuf.java
    # ${FLATBUFFERS}/com/google/flatbuffers/ReadWriteBuf.java
    # ${FLATBUFFERS}/com/google/flatbuffers/ShortVector.java
    # ${FLATBUFFERS}/com/google/flatbuffers/StringVector.java
    # ${FLATBUFFERS}/com/google/flatbuffers/Struct.java
    # ${FLATBUFFERS}/com/google/flatbuffers/Table.java
    # ${FLATBUFFERS}/com/google/flatbuffers/UnionVector.java
    # ${FLATBUFFERS}/com/google/flatbuffers/Utf8.java
    # ${FLATBUFFERS}/com/google/flatbuffers/Utf8Old.java
    # ${FLATBUFFERS}/com/google/flatbuffers/Utf8Safe.java
    OUTPUT_DIR ${PROJECT_BINARY_DIR}/java
    GENERATE_NATIVE_HEADERS db_jni_headers
    DESTINATION ${PROJECT_BINARY_DIR}/db/core/)
  get_target_property(GAIA_TINKERPOP_JAR GaiaTinkerpop JAR_FILE)

  # Java tests.
  # add_jar(GaiaTests
  #   SOURCES
  #     com/gaiaplatform/tests/database/cachegraph/TestCacheGraph.java
  #   INCLUDE_JARS GaiaTinkerpop)
  # get_target_property(GAIA_TESTS_JAR GaiaTests JAR_FILE)

  # Add JNI test.
  # This test is disabled for Debug builds because we could not find
  # libclang_rt.asan-x86.64.so.
  if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_test(
      NAME test_db_jni
      COMMAND
        daemonize $<TARGET_FILE:gaia_db_server_exec> --persistence disabled &;
        ${Java_JAVA_EXECUTABLE}
          -Djava.library.path=${PROJECT_BINARY_DIR}/db/core
          -cp ${GAIA_TINKERPOP_JAR}
          com.gaiaplatform.database.GaiaDatabase;
        pkill -f -KILL gaia_db_server)
  endif()

  # Add CacheGraph tests.
  # add_test(
  #   NAME test_cachegraph_basic
  #   COMMAND ${Java_JAVA_EXECUTABLE} -ea
  #     -Djava.library.path=${PROJECT_BINARY_DIR}/db/core
  #     -cp ${CMAKE_JAVA_INCLUDE_PATH}:${GAIA_TINKERPOP_JAR}:${GAIA_TESTS_JAR}
  #     com.gaiaplatform.tests.database.cachegraph.TestCacheGraph basic)
  # add_test(
  #   NAME test_cachegraph_factory
  #   COMMAND ${Java_JAVA_EXECUTABLE} -ea
  #     -Djava.library.path=${PROJECT_BINARY_DIR}/db/core
  #     -cp ${CMAKE_JAVA_INCLUDE_PATH}:${GAIA_TINKERPOP_JAR}:${GAIA_TESTS_JAR}
  #     com.gaiaplatform.tests.database.cachegraph.TestCacheGraph factory)
  # add_test(
  #   NAME test_cachegraph_graphml
  #   COMMAND ${Java_JAVA_EXECUTABLE} -ea
  #     -Djava.library.path=${PROJECT_BINARY_DIR}/db/core
  #     -cp ${CMAKE_JAVA_INCLUDE_PATH}:${GAIA_TINKERPOP_JAR}:${GAIA_TESTS_JAR}
  #     com.gaiaplatform.tests.database.cachegraph.TestCacheGraph graphml)
endif()
