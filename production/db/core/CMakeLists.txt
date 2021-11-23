#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

project(core)

set(GAIA_DB_CORE_PUBLIC_INCLUDES
  "${GAIA_INC}"
  "${GAIA_REPO}/production/db/inc/types"
)

message(VERBOSE "GAIA_DB_CORE_PUBLIC_INCLUDES=${GAIA_DB_CORE_PUBLIC_INCLUDES}")

set(GAIA_DB_CORE_PRIVATE_INCLUDES
  "${GAIA_DB_INC}/core"
  "${GAIA_DB_INC}/core/inc"
  "${GAIA_DB_INC}/inc/memory_manager"
  "${GAIA_REPO}/production/db/inc/memory_manager"
  "${GAIA_DB_INC}/payload_types"
  "${GAIA_DB_INC}/index"
  "${ROCKSDB_INC}"
  "${ROCKSDB_UTIL}"
  "${PROJECT_SOURCE_DIR}/inc"
  "${GAIA_DB_INC}/query_processor"
  "${GAIA_SPDLOG_INC}"
)

message(VERBOSE "GAIA_DB_CORE_PRIVATE_INCLUDES=${GAIA_DB_CORE_PRIVATE_INCLUDES}")

###############################################
# Generate flatbuffers headers.
###############################################
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

# The names of the following source files should be unique because they'll be used as targets.
set(FBS_SOURCES
  "${PROJECT_SOURCE_DIR}/src/flatbuffers/messages.fbs"
  "${PROJECT_SOURCE_DIR}/src/flatbuffers/gaia_table.fbs"
  "${PROJECT_SOURCE_DIR}/src/flatbuffers/gaia_field.fbs"
  "${PROJECT_SOURCE_DIR}/src/flatbuffers/gaia_relationship.fbs"
  "${PROJECT_SOURCE_DIR}/src/flatbuffers/gaia_index.fbs")

set(FBS_SOURCE_FILENAMES "")
foreach(FBS_SOURCE ${FBS_SOURCES})
  get_filename_component(FBS_SOURCE_FILENAME ${FBS_SOURCE} NAME)
  string(REGEX REPLACE "\\.fbs$" "_generated.h" GEN_HEADER ${FBS_SOURCE_FILENAME})

  add_custom_command(
    COMMENT "Compiling ${FBS_SOURCE} with flatbuffers..."
    OUTPUT "${GEN_DIR}/${GEN_HEADER}"
    COMMAND cmake -E make_directory "${GEN_DIR}"
    COMMAND ${FLATBUFFERS_BIN}/flatc --cpp --scoped-enums -o "${GEN_DIR}" "${FBS_SOURCE}"
    DEPENDS "${FBS_SOURCE}"
    DEPENDS flatc
    VERBATIM)

  add_custom_target(${FBS_SOURCE_FILENAME} ALL DEPENDS "${GEN_DIR}/${GEN_HEADER}")
  list(APPEND FBS_SOURCE_FILENAMES ${FBS_SOURCE_FILENAME})
endforeach(FBS_SOURCE)

###############################################
# Our main target.
###############################################
add_library(gaia_storage STATIC
  src/record_list.cpp
  src/record_list_manager.cpp)
configure_gaia_target(gaia_storage)
target_include_directories(gaia_storage PUBLIC ${GAIA_DB_CORE_PUBLIC_INCLUDES})
target_include_directories(gaia_storage PRIVATE ${GAIA_DB_CORE_PRIVATE_INCLUDES})
target_link_libraries(gaia_storage PRIVATE gaia_common)

###############################################
# Database-related code begins here.
###############################################
add_library(gaia_db_client STATIC
  src/catalog_core.cpp
  src/gaia_db.cpp
  src/gaia_ptr.cpp
  src/gaia_ptr_client.cpp
  src/payload_diff.cpp
  src/client_messenger.cpp
  src/mapped_data.cpp
  src/db_client.cpp
  src/db_shared_data_client.cpp
  src/type_metadata.cpp
  src/type_id_mapping.cpp
  src/db_client_config.cpp
  src/db_server_instance.cpp
  src/index_builder.cpp
  src/txn_metadata.cpp)

add_dependencies(gaia_db_client ${FBS_SOURCE_FILENAMES})
configure_gaia_target(gaia_db_client)
target_include_directories(gaia_db_client PUBLIC ${GAIA_DB_CORE_PUBLIC_INCLUDES})
target_include_directories(gaia_db_client PRIVATE ${GAIA_DB_CORE_PRIVATE_INCLUDES})
target_link_libraries(gaia_db_client PUBLIC gaia_common)
target_link_libraries(gaia_db_client PRIVATE gaia_payload_types gaia_memory_manager gaia_qp dac_catalog ${LIB_EXPLAIN} ${LIB_CAP} dl)

# Suppress spurious warnings about zero-initialized structs.
target_compile_options(gaia_db_client PRIVATE -Wno-missing-field-initializers)

# For flatbuffers generated code.
target_include_directories(gaia_db_client SYSTEM PRIVATE "${FLATBUFFERS_INC}")
target_include_directories(gaia_db_client SYSTEM PRIVATE "${GEN_DIR}")
target_link_libraries(gaia_db_client PUBLIC gaia_common)
target_link_libraries(gaia_db_client PRIVATE gaia_payload_types gaia_memory_manager gaia_index gaia_qp dac_catalog ${LIB_EXPLAIN} ${LIB_CAP} dl)

add_library(rocks_wrapper STATIC
  src/rdb_internal.cpp
  src/rdb_object_converter.cpp
  src/persistent_store_manager.cpp)
configure_gaia_target(rocks_wrapper)
target_include_directories(rocks_wrapper PRIVATE
  "${GAIA_DB_CORE_PUBLIC_INCLUDES}"
  "${GAIA_DB_CORE_PRIVATE_INCLUDES}")
target_include_directories(rocks_wrapper SYSTEM PRIVATE "${FLATBUFFERS_INC}")
target_include_directories(rocks_wrapper SYSTEM PRIVATE "${GEN_DIR}")
# The client is required to create gaia objects on recovery.
target_link_libraries(rocks_wrapper PUBLIC rocksdb gaia_common gaia_storage)

# Liburing
# Choose static version of the library.
message(CHECK_START "Looking for liburing")
find_path(LIBURING_INCLUDE_DIR NAMES liburing.h)
find_library(LIBURING_LIBRARY NAMES liburing.a)
if (LIBURING_LIBRARY)
  message(CHECK_PASS "found")
else()
  message(CHECK_FAIL "not found")
  message(FATAL_ERROR "liburing is required.")
endif()

add_library(gaia_db_persistence STATIC
  src/log_file.cpp
  src/async_disk_writer.cpp
  src/async_write_batch.cpp)
configure_gaia_target(gaia_db_persistence)
target_include_directories(gaia_db_persistence PRIVATE
  "${GAIA_DB_CORE_PUBLIC_INCLUDES}"
  "${GAIA_DB_CORE_PRIVATE_INCLUDES}")
target_include_directories(gaia_db_persistence SYSTEM PRIVATE "${FLATBUFFERS_INC}")
target_include_directories(gaia_db_persistence SYSTEM PRIVATE "${GEN_DIR}")
target_include_directories(gaia_db_persistence SYSTEM PRIVATE "${LIBURING_INCLUDE_DIR}")
# Only need util, not all of rocks.
target_link_libraries(gaia_db_persistence PUBLIC rocksdb "${LIBURING_LIBRARY}")

set(GAIA_DB_SERVER_SOURCES
  src/catalog_core.cpp
  src/db_server.cpp
  src/db_shared_data_server.cpp
  src/gaia_ptr.cpp
  src/gaia_ptr_server.cpp
  src/index_builder.cpp
  src/mapped_data.cpp
  src/system_checks.cpp
  src/txn_metadata.cpp
  src/type_generator.cpp
  src/type_id_mapping.cpp)

add_library(gaia_db_server ${GAIA_DB_SERVER_SOURCES})
add_dependencies(gaia_db_server ${FBS_SOURCE_FILENAMES})
configure_gaia_target(gaia_db_server)
target_include_directories(gaia_db_server PUBLIC
  "${GAIA_DB_CORE_PUBLIC_INCLUDES}"
  "${GAIA_DB_CORE_PRIVATE_INCLUDES}")
# For flatbuffers generated code.
target_include_directories(gaia_db_server SYSTEM PUBLIC "${FLATBUFFERS_INC}")
target_include_directories(gaia_db_server SYSTEM PUBLIC "${GEN_DIR}")

target_link_libraries(gaia_db_server
  PUBLIC
    ${LIB_CAP}
    ${LIB_EXPLAIN}
    Threads::Threads
    dl
    gaia_common
    gaia_db_persistence
    gaia_index
    gaia_memory_manager
    rocks_wrapper
)

add_executable(gaia_db_server_exec src/db_server_exec.cpp)
configure_gaia_target(gaia_db_server_exec)
set_target_properties(gaia_db_server_exec PROPERTIES OUTPUT_NAME gaia_db_server)
target_include_directories(gaia_db_server_exec PRIVATE
  "${GAIA_DB_CORE_PUBLIC_INCLUDES}"
  "${GAIA_DB_CORE_PRIVATE_INCLUDES}"
  "${GEN_DIR}")
target_link_libraries(gaia_db_server_exec PRIVATE gaia_db_server)
if(ENABLE_STACKTRACE)
  target_link_libraries(gaia_db_server_exec PRIVATE gaia_stack_trace)
endif()
install(TARGETS gaia_db_server_exec DESTINATION ${CMAKE_INSTALL_BINDIR})

set(GAIA_DB_CORE_TEST_INCLUDES
  ${GAIA_DB_CORE_PUBLIC_INCLUDES}
  ${GAIA_DB_CORE_PRIVATE_INCLUDES}
)

###############################################
# PyBind11 wrapper for database.
###############################################
find_package(pybind11 REQUIRED)

if(Python3_FOUND AND pybind11_FOUND)
  pybind11_add_module(gaia_db_pybind src/db_pybind_wrapper.cpp)
  configure_gaia_target(gaia_db_pybind)
  target_include_directories(gaia_db_pybind PRIVATE ${PROJECT_SOURCE_DIR})
  target_include_directories(gaia_db_pybind PRIVATE
    "${GAIA_DB_CORE_PUBLIC_INCLUDES}"
    "${GAIA_DB_CORE_PRIVATE_INCLUDES}")
  target_include_directories(gaia_db_pybind SYSTEM PRIVATE "${GEN_DIR}")
  target_link_libraries(gaia_db_pybind PRIVATE pybind11::module rt gaia_system gaia_db_client)

  # See https://github.com/pybind/pybind11/issues/1604 for inclusion of -fsized-deallocation for C++17
  target_compile_options(gaia_db_pybind PRIVATE -Wno-deprecated-register -Wno-unused-parameter -fsized-deallocation)

  if("$CACHE{SANITIZER}" STREQUAL "ASAN")
    target_link_options(gaia_db_pybind PRIVATE "-shared-libasan")
  endif()

  # Python wrapper unit test.
  # This test is disabled for Debug builds.
  # See: https://gaiaplatform.atlassian.net/browse/GAIAPLAT-602
  if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    configure_file(tests/test_db.py . COPYONLY)
    add_test(
      NAME test_db_pybind
      COMMAND ${Python3_EXECUTABLE} test_db.py)
  endif()
endif()

###############################################
# JNI wrapper for database.
###############################################
if(JAVA_FOUND AND JNI_FOUND)
  # Configure build of native C++ wrapper library.
  add_library(gaia_db_jni SHARED src/db_jni_wrapper.cpp)
  configure_gaia_target(gaia_db_jni)
  target_include_directories(gaia_db_jni PRIVATE ${PROJECT_SOURCE_DIR})
  target_include_directories(gaia_db_jni PRIVATE ${PROJECT_BINARY_DIR})
  target_include_directories(gaia_db_jni PRIVATE ${JNI_INCLUDE_DIRS})
  target_include_directories(gaia_db_jni PRIVATE ${GAIA_DB_CORE_INCLUDES})
  target_include_directories(gaia_db_jni PRIVATE ${GAIA_REPO}/production/db/inc/memory_manager)
  target_link_libraries(gaia_db_jni PRIVATE rt gaia_system gaia_db_client db_jni_headers)
  target_compile_options(gaia_db_jni PRIVATE -Wno-deprecated-register -Wno-unused-parameter)
  if("$CACHE{SANITIZER}" STREQUAL "ASAN")
    target_link_options(gaia_db_jni PRIVATE "-shared-libasan")
  endif()
endif()

set(GAIA_DB_CATALOG_TEST_PRIVATE_INCLUDES
  ${GAIA_INC}
  ${GAIA_REPO}/production/schemas/repository/inc
  ${GAIA_REPO}/production/db/core/inc
  ${GAIA_REPO}/production/db/inc/memory_manager
)

add_library(gaia_db_catalog_test tests/db_catalog_test_base.cpp)
configure_gaia_target(gaia_db_catalog_test)
target_include_directories(gaia_db_catalog_test PRIVATE ${GAIA_DB_CATALOG_TEST_PRIVATE_INCLUDES})
target_link_libraries(gaia_db_catalog_test PRIVATE gaia_common gaia_db_client gaia_schema_loader gtest)

add_gtest(test_rdb_object_converter tests/test_rdb_object_converter.cpp "${GEN_DIR};${GAIA_DB_CORE_TEST_INCLUDES}" "rocks_wrapper;gaia_db_server")
add_gtest(test_record_list tests/test_record_list.cpp "${GAIA_DB_CORE_TEST_INCLUDES}" "gaia_common;gaia_storage")
add_gtest(test_record_list_manager tests/test_record_list_manager.cpp "${GAIA_DB_CORE_TEST_INCLUDES}" "gaia_common;gaia_storage")
add_gtest(test_db_client tests/test_db_client.cpp "${GAIA_DB_CORE_TEST_INCLUDES}" "gaia_db_client;gaia_direct;gaia_catalog")
add_gtest(test_db_internals tests/test_db_internals.cpp "${GAIA_DB_CORE_TEST_INCLUDES}" "gaia_common;gaia_storage;gaia_db_client")
add_gtest(test_db_references tests/test_db_references.cpp "${GAIA_DB_CORE_TEST_INCLUDES}" "gaia_common;gaia_db_client;gaia_direct;gaia_catalog")
add_gtest(test_env_instance_name tests/test_db_server_env.cpp "${GAIA_DB_CORE_TEST_INCLUDES}" "gaia_common;gaia_db_client;gaia_direct")

add_gtest(test_multiple_server_instances
    tests/test_multiple_server_instances.cpp
    "${GEN_DIR};${GAIA_DB_CORE_TEST_INCLUDES}"
    "gaia_common;gaia_db_client;gaia_direct;gaia_schema_loader;rt;dac_addr_book")

add_gtest(test_relationships
  tests/test_relationships.cpp
  "${GAIA_DB_CORE_TEST_INCLUDES};${GAIA_INC};${FLATBUFFERS_INC}"
  "gaia_common;gaia_db_client;gaia_catalog")

add_gtest(test_catalog_core
  tests/test_catalog_core.cpp
  "${FLATBUFFERS_INC};${GAIA_DB_CORE_TEST_INCLUDES};${GAIA_INC}"
  "gaia_common;gaia_db_client;gaia_catalog")
