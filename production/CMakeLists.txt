#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.17)

include(UseJava)

enable_testing()

# Set the project name.
project(production)

option(EXECUTE_FDW_TESTS "Execute FDW tests" OFF)

option(BUILD_GAIA_RELEASE "Build Gaia Release packages and binaries" OFF)
include(GNUInstallDirs)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

include(gaia)
include(GoogleTest)

# Specify the C++ standard.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Ensure debug builds have the DEBUG flag defined.
add_compile_options("$<$<CONFIG:DEBUG>:-DDEBUG>")

# Set some global variables.
get_repo_root(${PROJECT_SOURCE_DIR} GAIA_REPO)
if(BUILD_GAIA_RELEASE)
  set(GAIA_COMPILE_FLAGS "-c -Wall -Wextra")
  set(GAIA_LINK_FLAGS "-pthread")
else()
  set(GAIA_COMPILE_FLAGS "-c -Wall -Wextra -ggdb")
  set(GAIA_LINK_FLAGS "-ggdb -pthread")
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(GAIA_COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
  set(GAIA_LINK_FLAGS "${GAIA_LINK_FLAGS} -fsanitize=address")
endif()

message(VERBOSE "GAIA_COMPILE_FLAGS=${GAIA_COMPILE_FLAGS}")
message(VERBOSE "GAIA_LINK_FLAGS=${GAIA_LINK_FLAGS}")

set(GAIA_INC "${PROJECT_SOURCE_DIR}/inc")
set(GAIA_INTERNAL_INC "${PROJECT_SOURCE_DIR}/inc/gaia_internal")

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

set(GAIA_CONFIG "${PROJECT_SOURCE_DIR}/gaia.conf")
set(GAIA_LOG_CONFIG "${PROJECT_SOURCE_DIR}/gaia_log.conf")

find_library(LIB_EXPLAIN NAMES libexplain.a)
find_library(LIB_CAP NAMES libcap.a cap)

if (LIB_EXPLAIN AND LIB_CAP)
  message(CHECK_PASS "found")
else()
  message(CHECK_FAIL "not found")
  message(FATAL_ERROR "libexplain and libcap are required.")
endif()


# Force spdlog to use an external version of fmt.
# TODO this flag should only be set in common/CMakeLists.txt. The flag is
#  necessary here too, otherwise the compilation fail. The reason could
#  be that logger.hpp imports the spdlog headers, therefore, every file
#  that imports logger.hpp, also imports spdlog.h, which imports fmt.h.
#  Each time spdlog need to evaluate the flag SPDLOG_FMT_EXTERNAL to
#  decide whether to use the external version of fmt.
set(GAIA_COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -DSPDLOG_FMT_EXTERNAL")

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

# External project dependencies.

# Rocksdb.
add_subdirectory(${ROCKSDB} rocksdb EXCLUDE_FROM_ALL)
# Flatbuffers.
add_subdirectory(${FLATBUFFERS} flatbuffers)
set_property (TARGET flatbuffers PROPERTY POSITION_INDEPENDENT_CODE ON)

include(ExternalProject)
set(DEPENDENCIES_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/deps)

# Backward.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_subdirectory("${GAIA_REPO}/third_party/production/backward" backward)
endif()
# Cpptoml.
add_subdirectory("${GAIA_REPO}/third_party/production/cpptoml" cpptoml)
# Fmt.
add_subdirectory("${GAIA_REPO}/third_party/production/fmt" fmt)
# Spdlog.
add_subdirectory("${GAIA_REPO}/third_party/production/spdlog" spdlog)
# Spdlog_setup.
add_subdirectory("${GAIA_REPO}/third_party/production/spdlog_setup" spdlog_setup)
# Tabulate.
add_subdirectory("${GAIA_REPO}/third_party/production/tabulate" tabulate)
# Googletest
add_subdirectory(${GOOGLE_TEST} googletest EXCLUDE_FROM_ALL)

# Setting clang-tidy here to avoid it being applied to flatbuffers, rocksdb etc...
# TODO Maybe we could force each subproject to set this flag?
set(CMAKE_CXX_CLANG_TIDY clang-tidy)

# Add individual component folders.
# These are listed roughly in a bottom-up order
# in terms of platform architecture.

# Core database engine.
add_subdirectory(common)
add_subdirectory(db)
add_subdirectory(system)

# Direct access API.
add_subdirectory(direct_access)

# Catalog.
add_subdirectory(catalog)

# Rules engine.
add_subdirectory(rules)
add_subdirectory(schemas)

# Tools.
add_subdirectory(tools)


if(BUILD_GAIA_RELEASE)
  unset(CMAKE_CXX_CLANG_TIDY)
  add_subdirectory("${GAIA_REPO}/third_party/production/TranslationEngineLLVM/llvm" llvm EXCLUDE_FROM_ALL)
  set(CMAKE_CXX_CLANG_TIDY clang-tidy)
endif()

# Query processors.
add_subdirectory(sql)

# SDK
# Be sure to add the SDK directory after the sql directory because the package mechanism
# requires that we know whether the gaia_fdw target will be added or not. If it is added
# then we can add gaia_fdw to the list of dependencies for the package.
add_subdirectory(sdk)

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
    GENERATE_NATIVE_HEADERS db_jni_headers
      DESTINATION ${PROJECT_BINARY_DIR}/db/core/)
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
  #     -Djava.library.path=${PROJECT_BINARY_DIR}/db/core
  #     -cp ${GAIA_TINKERPOP_JAR}
  #     com.gaiaplatform.database.CowStorageEngine)

  # Add CacheGraph tests.
#   add_test(
#     NAME test_cachegraph_basic
#     COMMAND ${Java_JAVA_EXECUTABLE} -ea
#       -Djava.library.path=${PROJECT_BINARY_DIR}/db/core
#       -cp ${CMAKE_JAVA_INCLUDE_PATH}:${GAIA_TINKERPOP_JAR}:${GAIA_TESTS_JAR}
#       com.gaiaplatform.tests.database.cachegraph.TestCacheGraph basic)
#   add_test(
#     NAME test_cachegraph_factory
#     COMMAND ${Java_JAVA_EXECUTABLE} -ea
#       -Djava.library.path=${PROJECT_BINARY_DIR}/db/core
#       -cp ${CMAKE_JAVA_INCLUDE_PATH}:${GAIA_TINKERPOP_JAR}:${GAIA_TESTS_JAR}
#       com.gaiaplatform.tests.database.cachegraph.TestCacheGraph factory)
#   add_test(
#     NAME test_cachegraph_graphml
#     COMMAND ${Java_JAVA_EXECUTABLE} -ea
#       -Djava.library.path=${PROJECT_BINARY_DIR}/db/core
#       -cp ${CMAKE_JAVA_INCLUDE_PATH}:${GAIA_TINKERPOP_JAR}:${GAIA_TESTS_JAR}
#       com.gaiaplatform.tests.database.cachegraph.TestCacheGraph graphml)
endif()

# Customize test commands.
file(GENERATE OUTPUT "${PROJECT_BINARY_DIR}/CTestCustom.cmake"
  CONTENT "set(CTEST_CUSTOM_PRE_TEST \"daemonize $<TARGET_FILE:gaia_db_server> --disable-persistence\")\nset(CTEST_CUSTOM_POST_TEST \"pkill -f -KILL gaia_db_server\")\n"
)

# Copy log configuration file to make it visible during
# common/ tests (currently not in use). Leaving this for reference
file(
  COPY ${CMAKE_CURRENT_SOURCE_DIR}/gaia_log.conf
  DESTINATION ${PROJECT_BINARY_DIR}/common/
)

# Copy the git-hooks. git-hooks are local to any git repo and
# are not copied over when you clone a repo.
file(
  COPY ${CMAKE_CURRENT_SOURCE_DIR}/../dev_tools/hooks/pre-commit
  DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/../.git/hooks
)

# We want to disable all clang-tidy checks for generated code.
# However, if no checks are specified, clang-tidy will assume we
# did not configure it correctly. Just add one check that will never be found.
file(WRITE ${GAIA_PROD_BUILD}/.clang-tidy "Checks: '-*,llvm-twine-local'\n")

# Generate dependencies graph
# https://cmake.org/cmake/help/latest/module/CMakeGraphVizOptions.html
# To visualize the results you can:
# - Use dot to generate a png/svg (dot -Tpng -o foo.png foo.dot)
# - Install an IDE plugin that automatically renders graphviz files
# - Use an online renderer such as: https://dreampuf.github.io/GraphvizOnline
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CMakeGraphVizOptions.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/CMakeGraphVizOptions.cmake COPYONLY)

add_custom_target(gaia_dependency_graph
  COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/gaia_dependencies
  COMMAND rm -f ${CMAKE_CURRENT_BINARY_DIR}/gaia_dependencies/*
  COMMAND "${CMAKE_COMMAND}" "--graphviz=gaia_dependencies/gaia" .
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
