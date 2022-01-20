#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.17)

# Specify the C++ standard.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(UseJava)

enable_testing()

project(production VERSION 0.3.3)

# Sets the pre-release version which will be appended to the
# product version: 0.1.0-alpha.
# When the product is GA this should be empty.
set(PRE_RELEASE_IDENTIFIER "beta")

option(EXECUTE_FDW_TESTS "Execute FDW tests" OFF)

option(ENABLE_STACKTRACE "Enable stack traces" ON)

option(BUILD_GAIA_SDK "Build Gaia Release packages and binaries" OFF)

option(ENABLE_SDK_TESTS "Build and execute ruleset translation tests" ON)

option(EXPORT_SYMBOLS "Export symbols by default from shared libraries" OFF)

option(DISABLE_ASSERTS "Disable assertions" OFF)

option(DISABLE_CCACHE "Disable ccache unconditionally" OFF)

option(ENABLE_CLANG_TIDY "Enable Clang-Tidy to be specified externally")

if (NOT ENABLE_CLANG_TIDY)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    option(ENABLE_CLANG_TIDY "Enable clang tidy" ON)
  else()
    option(ENABLE_CLANG_TIDY "Enable clang tidy" OFF)
  endif()
endif()

# Sets the Gaia build architecture which is the SDK target architecture.
# For simplicity we do not support cross-compiling yet.
if (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "amd64|AMD64|x86_64|X86_64")
  set(GAIA_BUILD_ARCH "amd64")
elseif (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64|ARM64|aarch64|AARCH64")
  set(GAIA_BUILD_ARCH "arm64")
else()
  message(FATAL_ERROR "Unrecognised architecture: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
endif()

message(STATUS "GAIA_BUILD_ARCH: ${GAIA_BUILD_ARCH}")

include(GNUInstallDirs)
include(ExternalProject)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

include(gaia_internal)
include(GoogleTest)

# Check that PIE flags are actually passed to the linker.
# This ensures that our executables are ASLR-compatible.
include(CheckPIESupported)
check_pie_supported(OUTPUT_VARIABLE check_pie_error LANGUAGES CXX)
if(NOT CMAKE_CXX_LINK_PIE_SUPPORTED)
  message(FATAL_ERROR "PIE is not supported at link time: ${check_pie_error}")
endif()
# See https://cmake.org/cmake/help/latest/policy/CMP0083.html#policy:CMP0083.
# The NEW behavior under this policy is to pass the -pie flag to the linker if
# POSITION_INDEPENDENT_CODE is set.
cmake_policy(SET CMP0083 NEW)
# Our shared libraries and executables must all be ASLR-compatible.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Set some global variables.
get_repo_root(${PROJECT_SOURCE_DIR} GAIA_REPO)

# Default to Release builds if not specified on the command line.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND AND NOT DISABLE_CCACHE)
  set_property(GLOBAL PROPERTY CMAKE_CXX_COMPILER_LAUNCHER ccache)
endif()

# Default compiler/linker flags for Gaia targets.
add_library(gaia_build_options INTERFACE)
target_compile_options(gaia_build_options INTERFACE -Wall -Wextra -ftime-trace)
# Ensure Debug builds have the DEBUG macro defined.
target_compile_definitions(gaia_build_options INTERFACE $<$<CONFIG:Debug>:DEBUG>)
# Propagate the DISABLE_ASSERTS option to a macro definition.
target_compile_definitions(gaia_build_options INTERFACE $<$<BOOL:${DISABLE_ASSERTS}>:DISABLE_ASSERTS>)
# Disabling asserts will produce bogus unused variable warnings.
target_compile_options(gaia_build_options INTERFACE $<$<BOOL:${DISABLE_ASSERTS}>:-Wno-unused-parameter -Wno-unused-lambda-capture -Wno-unused-variable -Wno-unused-but-set-variable>)

# LLVM and CLANG specific options.
if(BUILD_GAIA_SDK OR BUILD_GAIA_LLVM_TESTS)
  set(LLVM_ENABLE_WARNINGS OFF CACHE BOOL "")
  set(CLANG_BUILD_TOOLS OFF CACHE BOOL "")
  set(LLVM_BUILD_TOOLS OFF CACHE BOOL "")
  set(LLVM_ENABLE_EH ON CACHE BOOL "")
  set(LLVM_ENABLE_PROJECTS "clang" CACHE STRING "")
  set(LLVM_ENABLE_RTTI ON CACHE BOOL "")
  # LLD should make building LLVM significantly faster.
  set(LLVM_ENABLE_LLD ON CACHE BOOL "")

  if(ENABLE_STACKTRACE)
    set(LLVM_GENERATE_STACKTRACE_DEBUG_INFO ON CACHE BOOL "")
  endif()

  if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    # This speeds up linking by separating debug info.
    set(LLVM_USE_SPLIT_DWARF ON CACHE BOOL "")
    # This makes debugger startup faster.
    set(LLVM_USE_GDB_INDEX ON CACHE BOOL "")
    # This saves the linker some work, but makes clang slower.
    set(BUILD_SHARED_LIBS:BOOL ON CACHE BOOL "")
  endif()
endif()

# Debug build-specific options.
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  # Don't omit debug symbols from any built binaries.
  add_compile_options(-fno-limit-debug-info)

  target_compile_options(gaia_build_options INTERFACE -g3 -ggdb -fno-omit-frame-pointer -fno-optimize-sibling-calls -ggnu-pubnames -gsplit-dwarf)
  target_link_options(gaia_build_options INTERFACE -Wl,--gdb-index)

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(gaia_build_options INTERFACE -O0)

    # For eventual MemorySanitizer support, we need all targets in debug builds
    # to use libc++, including third-party packages.
    add_compile_options(-stdlib=libc++)
    add_link_options(-stdlib=libc++)

    # Enable ASan by default in debug builds.
    set(SANITIZER "ASAN" CACHE STRING "Enable sanitizers in debug builds")

    # Explicitly list all legal values for SANITIZER.
    # (This will eventually include ASAN, MSAN, TSAN.)
    set_property(CACHE SANITIZER PROPERTY STRINGS "ASAN" "TSAN" "NONE")
  endif()

  if(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    # Set the highest optimization level compatible with reasonable debuggability.
    target_compile_options(gaia_build_options INTERFACE -Og)
  endif()
else()
  # Set the maximum optimization level for release builds.
  target_compile_options(gaia_build_options INTERFACE -O3)
endif()

# Sanitizer-specific compiler/linker flags.
if(DEFINED CACHE{SANITIZER})
  message(VERBOSE "Sanitizer=$CACHE{SANITIZER}")

  # Sanitizers can only be defined in debug builds.
  if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Clear the given value from the cache so subsequent runs don't fail.
    unset(SANITIZER CACHE)
    message(FATAL_ERROR "The SANITIZER option can only be set in debug builds.")
  endif()

  # Extract allowed option values.
  get_property(SANITIZER_OPTIONS CACHE SANITIZER PROPERTY STRINGS)

  # Check that value of the option is in the list of allowed values.
  if(NOT "$CACHE{SANITIZER}" IN_LIST SANITIZER_OPTIONS)
    # Clear a bad value from the cache so subsequent runs don't fail.
    set(bad_sanitizer "$CACHE{SANITIZER}")
    unset(SANITIZER CACHE)
    message(FATAL_ERROR "Unrecognized value for SANITIZER option: ${bad_sanitizer}.\n"
      "Allowed values: ${SANITIZER_OPTIONS}.")
  endif()

  # Set compiler/linker flags that apply to all sanitizers.
  add_compile_options(-fno-sanitize-recover=all)
  add_link_options(-fno-sanitize-recover=all)

  if("$CACHE{SANITIZER}" STREQUAL "ASAN")
    # Enable ASan on all targets because libc++ requires it to prevent false positives.
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)

    # Add UBSan as well as ASan to Gaia targets.
    # (They are compatible so we have no separate option for UBSan.)
    target_compile_options(gaia_build_options INTERFACE -fsanitize=undefined)
    target_link_options(gaia_build_options INTERFACE -fsanitize=undefined)
  endif()

  if("$CACHE{SANITIZER}" STREQUAL "TSAN")
    # Add TSan flags only to Gaia targets to avoid compatibility issues with other libraries.
    target_compile_options(gaia_build_options INTERFACE -fsanitize=thread)
    target_link_options(gaia_build_options INTERFACE -fsanitize=thread)
  endif()
endif()

message(STATUS "CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
get_target_property(gaia_compile_options gaia_build_options INTERFACE_COMPILE_OPTIONS)
get_target_property(gaia_link_options gaia_build_options INTERFACE_LINK_OPTIONS)
message(VERBOSE "Gaia compile options: ${gaia_compile_options}")
message(VERBOSE "Gaia link options: ${gaia_link_options}")

set(GAIA_INC "${PROJECT_SOURCE_DIR}/inc")
set(DEPENDENCIES_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/deps")

set(GOOGLE_TEST "${GAIA_REPO}/third_party/production/googletest")
set(GOOGLE_TEST_INC "${GOOGLE_TEST}/googletest/include")

set(FLATBUFFERS_ROOT ${DEPENDENCIES_INSTALL_PREFIX}/flatbuffers)
set(FLATBUFFERS_SRC "${FLATBUFFERS_ROOT}/src")
set(FLATBUFFERS_INC "${FLATBUFFERS_ROOT}/include")
set(FLATBUFFERS_BIN "${FLATBUFFERS_ROOT}/bin")
set(FLATBUFFERS_LIB "${FLATBUFFERS_ROOT}/lib")

set(GAIA_SPDLOG "${DEPENDENCIES_INSTALL_PREFIX}/gaia_spdlog")
set(GAIA_SPDLOG_INC "${GAIA_SPDLOG}/include")

set(JSON_INC "${GAIA_REPO}/third_party/production/json")

set(ROCKSDB "${GAIA_REPO}/third_party/production/rocksdb")
set(ROCKSDB_INC "${ROCKSDB}/include")
set(ROCKSDB_UTIL "${ROCKSDB}/util")

set(GREMLIN_CONSOLE_PATH "/usr/local/share/gremlin-console")
set(GREMLIN_SERVER_PATH "/usr/local/share/gremlin-server")

set(GAIA_PROD_BUILD "${CMAKE_CURRENT_BINARY_DIR}")

set(GAIA_PARSER_GENERATED "${GAIA_PROD_BUILD}/catalog/parser/generated")
set(GAIA_GENERATED_CODE "${GAIA_PROD_BUILD}/gaia_generated")

set(GAIA_CONFIG "${PROJECT_SOURCE_DIR}/gaia.conf")
set(GAIA_LOG_CONFIG "${PROJECT_SOURCE_DIR}/gaia_log.conf")

# We need pthreads support.
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

message(CHECK_START "Looking for libexplain and libcap")
find_library(LIB_EXPLAIN NAMES libexplain.a)
find_library(LIB_CAP NAMES libcap.a cap)
if(LIB_EXPLAIN AND LIB_CAP)
  message(CHECK_PASS "found")
else()
  message(CHECK_FAIL "not found")
  message(FATAL_ERROR "libexplain and libcap are required.")
endif()

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

# Backward (this needs to come first because it changes build options for subsequent dependencies).
if(ENABLE_STACKTRACE)
  message(VERBOSE "ENABLE_STACKTRACE=${ENABLE_STACKTRACE}")
  add_subdirectory("${GAIA_REPO}/third_party/production/backward" backward)
  add_library(gaia_stack_trace INTERFACE)
  target_sources(gaia_stack_trace INTERFACE "${GAIA_INC}/gaia_internal/common/backward.cpp")
  add_compile_options(-funwind-tables -fno-omit-frame-pointer)
  # Only append this option for Release builds, or it will disable all other debug info generation.
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-gline-tables-only)
  endif()
  target_link_libraries(gaia_stack_trace INTERFACE backward)
endif()

# Flatbuffers.
add_subdirectory("${GAIA_REPO}/third_party/production/flatbuffers" flatbuffers)

# If this add_executable is placed into third_party/production/flatbuffers/CMakeLists.txt
# the rest of the project can't see it. This is the workaround I came up with, looking for a
# cleaner solution.
add_executable(flatc IMPORTED)
set_target_properties(flatc PROPERTIES IMPORTED_LOCATION ${FLATBUFFERS_BIN}/flatc)
add_dependencies(flatc flatbuffers_dependency)

# Rocksdb.
set(DISABLE_WARNINGS ON CACHE BOOL "" FORCE)
add_subdirectory(${ROCKSDB} rocksdb EXCLUDE_FROM_ALL)

# Cpptoml.
add_subdirectory("${GAIA_REPO}/third_party/production/cpptoml" cpptoml)
# Spdlog.
add_subdirectory("${GAIA_REPO}/third_party/production/gaia_spdlog" gaia_spdlog)
# Spdlog_setup.
add_subdirectory("${GAIA_REPO}/third_party/production/gaia_spdlog_setup" gaia_spdlog_setup)
# Tabulate.
add_subdirectory("${GAIA_REPO}/third_party/production/tabulate" tabulate)
# Googletest.
add_subdirectory(${GOOGLE_TEST} googletest EXCLUDE_FROM_ALL)

# Setting clang-tidy here to avoid it being applied to flatbuffers, rocksdb etc...
# TODO Maybe we could force each subproject to set this flag?
if(ENABLE_CLANG_TIDY)
    set(CMAKE_CXX_CLANG_TIDY clang-tidy)
endif()

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

if(BUILD_GAIA_SDK)
  unset(CMAKE_CXX_CLANG_TIDY)
  add_subdirectory("${GAIA_REPO}/third_party/production/TranslationEngineLLVM/llvm" llvm EXCLUDE_FROM_ALL)
  if(ENABLE_CLANG_TIDY)
    set(CMAKE_CXX_CLANG_TIDY clang-tidy)
  endif()
endif()

# Query processors.
# We want to export all global symbols from the FDW shared library.
set(EXPORT_SYMBOLS ON)
add_subdirectory(sql)
set(EXPORT_SYMBOLS OFF)

# SDK
# Be sure to add the SDK directory after the sql directory because the package mechanism
# requires that we know whether the gaia_fdw target will be added or not. If it is added
# then we can add gaia_fdw to the list of dependencies for the package.
add_subdirectory(sdk)

# Java Tinkerpop project.
if(JAVA_FOUND AND JNI_FOUND)
  # set(CMAKE_JAVA_INCLUDE_PATH ${GREMLIN_CONSOLE_PATH}/lib/*:${GREMLIN_CONSOLE_PATH}/ext/tinkergraph-gremlin/lib/*)

  # Java Tinkerpop graph provider implementation. This also generates native headers for the database wrapper.
  add_jar(GaiaTinkerpop
    SOURCES
      java/com/gaiaplatform/database/GaiaDatabase.java
      # java/com/gaiaplatform/database/Airline.java
      # java/com/gaiaplatform/database/Airport.java
      # java/com/gaiaplatform/database/Route.java
      # java/com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheEdge.java
      # java/com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheElement.java
      # java/com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheFactory.java
      # java/com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheGraph.java
      # java/com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheHelper.java
      # java/com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheProperty.java
      # java/com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheTransaction.java
      # java/com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheVertex.java
      # java/com/gaiaplatform/database/cachegraph/tinkerpop/gremlin/structure/CacheVertexProperty.java
      # ${FLATBUFFERS}/java/com/google/flatbuffers/ArrayReadWriteBuf.java
      # ${FLATBUFFERS}/java/com/google/flatbuffers/BaseVector.java
      # ${FLATBUFFERS}/java/com/google/flatbuffers/BooleanVector.java
      # ${FLATBUFFERS}/java/com/google/flatbuffers/ByteBufferReadWriteBuf.java
      # ${FLATBUFFERS}/java/com/google/flatbuffers/ByteBufferUtil.java
      # ${FLATBUFFERS}/java/com/google/flatbuffers/ByteVector.java
      # ${FLATBUFFERS}/java/com/google/flatbuffers/Constants.java
      # ${FLATBUFFERS}/java/com/google/flatbuffers/DoubleVector.java
      # ${FLATBUFFERS}/java/com/google/flatbuffers/FlatBufferBuilder.java
      # ${FLATBUFFERS}/java/com/google/flatbuffers/FlexBuffers.java
      # ${FLATBUFFERS}/java/com/google/flatbuffers/FlexBuffersBuilder.java
      # ${FLATBUFFERS}/java/com/google/flatbuffers/FloatVector.java
      # ${FLATBUFFERS}/java/com/google/flatbuffers/IntVector.java
      # ${FLATBUFFERS}/java/com/google/flatbuffers/LongVector.java
      # ${FLATBUFFERS}/java/com/google/flatbuffers/ReadBuf.java
      # ${FLATBUFFERS}/java/com/google/flatbuffers/ReadWriteBuf.java
      # ${FLATBUFFERS}/java/com/google/flatbuffers/ShortVector.java
      # ${FLATBUFFERS}/java/com/google/flatbuffers/StringVector.java
      # ${FLATBUFFERS}/java/com/google/flatbuffers/Struct.java
      # ${FLATBUFFERS}/java/com/google/flatbuffers/Table.java
      # ${FLATBUFFERS}/java/com/google/flatbuffers/UnionVector.java
      # ${FLATBUFFERS}/java/com/google/flatbuffers/Utf8.java
      # ${FLATBUFFERS}/java/com/google/flatbuffers/Utf8Old.java
      # ${FLATBUFFERS}/java/com/google/flatbuffers/Utf8Safe.java
    OUTPUT_DIR ${PROJECT_BINARY_DIR}/java
    GENERATE_NATIVE_HEADERS db_jni_headers
      DESTINATION ${PROJECT_BINARY_DIR}/db/core/)
  get_target_property(GAIA_TINKERPOP_JAR GaiaTinkerpop JAR_FILE)

  # Java tests.
  # add_jar(GaiaTests
  #   SOURCES
  #     java/com/gaiaplatform/tests/database/cachegraph/TestCacheGraph.java
  #   INCLUDE_JARS GaiaTinkerpop)
  # get_target_property(GAIA_TESTS_JAR GaiaTests JAR_FILE)

  # Add JNI test.
  # This test is disabled for Debug builds.
  # See: https://gaiaplatform.atlassian.net/browse/GAIAPLAT-602
  if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_test(
      NAME test_db_jni
      COMMAND ${Java_JAVA_EXECUTABLE}
        -Djava.library.path=${PROJECT_BINARY_DIR}/db/core
        -cp ${GAIA_TINKERPOP_JAR}
        com.gaiaplatform.database.GaiaDatabase)
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

# Handle Gaia versioning.
include(cmake/gaia_version.cmake)

# Customize test commands.
file(GENERATE OUTPUT "${PROJECT_BINARY_DIR}/CTestCustom.cmake"
  CONTENT "set(CTEST_CUSTOM_PRE_TEST \"daemonize $<TARGET_FILE:gaia_db_server_exec> --persistence disabled\")\nset(CTEST_CUSTOM_POST_TEST \"pkill -f -KILL gaia_db_server\")\n"
)

# Copy log configuration file to make it visible during
# common/ tests (currently not in use). Leaving this for reference
file(
  COPY ${CMAKE_CURRENT_SOURCE_DIR}/gaia_log.conf
  DESTINATION ${PROJECT_BINARY_DIR}/common/
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
