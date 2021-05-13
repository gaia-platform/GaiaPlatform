#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(core)

# We need pthreads support.
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

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
  "${ROCKSDB_INC}"
  "${PROJECT_SOURCE_DIR}/inc"
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
  "${PROJECT_SOURCE_DIR}/src/flatbuffers/gaia_relationship.fbs")

foreach(FBS_SOURCE ${FBS_SOURCES})
  get_filename_component(FBS_SOURCE_FILENAME ${FBS_SOURCE} NAME)
  string(REGEX REPLACE "\\.fbs$" "_generated.h" GEN_HEADER ${FBS_SOURCE_FILENAME})

  add_custom_command(
    COMMENT "Compiling ${FBS_SOURCE} with flatbuffers..."
    OUTPUT "${GEN_DIR}/${GEN_HEADER}"
    COMMAND cmake -E make_directory "${GEN_DIR}"
    COMMAND flatc --cpp --scoped-enums -o "${GEN_DIR}" "${FBS_SOURCE}"
    DEPENDS "${FBS_SOURCE}"
    DEPENDS ${CMAKE_BINARY_DIR}/flatbuffers/flatc
    VERBATIM)

  add_custom_target(${FBS_SOURCE_FILENAME} ALL DEPENDS "${GEN_DIR}/${GEN_HEADER}")
endforeach(FBS_SOURCE)

###############################################
# Our main target.
###############################################
add_library(gaia_storage STATIC
  src/record_list.cpp
  src/record_list_manager.cpp)
target_include_directories(gaia_storage PUBLIC ${GAIA_DB_CORE_PUBLIC_INCLUDES})
target_include_directories(gaia_storage PRIVATE ${GAIA_DB_CORE_PRIVATE_INCLUDES})
set_target_properties(gaia_storage PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(gaia_storage PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
target_link_libraries(gaia_storage PRIVATE gaia_common)

###############################################
# Database-related code begins here.
###############################################
add_library(gaia_db_client STATIC
  src/catalog_core.cpp
  src/gaia_db.cpp
  src/gaia_ptr.cpp
  src/payload_diff.cpp
  src/client_messenger.cpp
  src/mapped_data.cpp
  src/db_client.cpp
  src/db_shared_data_client.cpp
  src/type_metadata.cpp
  src/type_id_mapping.cpp)
add_dependencies(gaia_db_client "messages.fbs" "gaia_table.fbs" "gaia_field.fbs" "gaia_relationship.fbs")
target_include_directories(gaia_db_client PUBLIC ${GAIA_DB_CORE_PUBLIC_INCLUDES})
target_include_directories(gaia_db_client PRIVATE ${GAIA_DB_CORE_PRIVATE_INCLUDES})

# For flatbuffers generated code.
target_include_directories(gaia_db_client SYSTEM PRIVATE "${FLATBUFFERS_INC}")
target_include_directories(gaia_db_client SYSTEM PRIVATE "${GEN_DIR}")
# Suppress spurious warnings about zero-initialized structs.
set_target_properties(gaia_db_client PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -Wno-missing-field-initializers")
set_target_properties(gaia_db_client PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
target_link_libraries(gaia_db_client PUBLIC gaia_common)
target_link_libraries(gaia_db_client PRIVATE gaia_payload_types gaia_memory_manager ${LIB_EXPLAIN} ${LIB_CAP} dl)
# This static library must be compatible with linking in a shared library.
set_property (TARGET gaia_db_client PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(rocks_wrapper STATIC
  src/rdb_internal.cpp
  src/rdb_object_converter.cpp
  src/persistent_store_manager.cpp)
target_include_directories(rocks_wrapper PRIVATE
  "${GAIA_DB_CORE_PUBLIC_INCLUDES}"
  "${GAIA_DB_CORE_PRIVATE_INCLUDES}")
target_include_directories(rocks_wrapper SYSTEM PRIVATE "${FLATBUFFERS_INC}")
target_include_directories(rocks_wrapper SYSTEM PRIVATE "${GEN_DIR}")
# The client is required to create gaia objects on recovery.
target_link_libraries(rocks_wrapper rocksdb gaia_storage)
set_target_properties(rocks_wrapper PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -Wno-missing-field-initializers")
set_target_properties(rocks_wrapper PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
set_property (TARGET rocks_wrapper PROPERTY POSITION_INDEPENDENT_CODE ON)

set(GAIA_DB_SERVER_SOURCES
  src/mapped_data.cpp
  src/txn_metadata.cpp
  src/db_server.cpp
  src/db_shared_data_server.cpp
  src/db_server_exec.cpp
)

if(ENABLE_STACKTRACE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
  list(APPEND GAIA_DB_SERVER_SOURCES src/backward.cpp)
endif()

add_executable(gaia_db_server ${GAIA_DB_SERVER_SOURCES})
add_dependencies(gaia_db_server "messages.fbs")
target_include_directories(gaia_db_server PRIVATE
  "${GAIA_DB_CORE_PUBLIC_INCLUDES}"
  "${GAIA_DB_CORE_PRIVATE_INCLUDES}")

# For flatbuffers generated code.
target_include_directories(gaia_db_server SYSTEM PRIVATE "${FLATBUFFERS_INC}")
target_include_directories(gaia_db_server SYSTEM PRIVATE "${GEN_DIR}")

# Suppress spurious warnings about zero-initialized structs.
set_target_properties(gaia_db_server PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -Wno-missing-field-initializers")
set_target_properties(gaia_db_server PROPERTIES LINK_FLAGS ${GAIA_LINK_FLAGS})
target_link_libraries(gaia_db_server PRIVATE gaia_common Threads::Threads rocks_wrapper gaia_memory_manager ${LIB_EXPLAIN} ${LIB_CAP} dl)
if(ENABLE_STACKTRACE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
  target_link_libraries(gaia_db_server PUBLIC backward)
endif()
install(TARGETS gaia_db_server DESTINATION ${CMAKE_INSTALL_BINDIR})

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
  target_include_directories(gaia_db_pybind PRIVATE ${PROJECT_SOURCE_DIR})
  target_include_directories(gaia_db_pybind PRIVATE
    "${GAIA_DB_CORE_PUBLIC_INCLUDES}"
    "${GAIA_DB_CORE_PRIVATE_INCLUDES}")
  target_include_directories(gaia_db_pybind SYSTEM PRIVATE "${GEN_DIR}")
  target_link_libraries(gaia_db_pybind PRIVATE pybind11::module rt gaia_system gaia_db_client)
  # See https://github.com/pybind/pybind11/issues/1604 for inclusion of -fsized-deallocation for C++17
  set_target_properties(gaia_db_pybind PROPERTIES COMPILE_FLAGS
    "${GAIA_COMPILE_FLAGS} -Wno-deprecated-register -Wno-unused-parameter -fsized-deallocation")
  set_target_properties(gaia_db_pybind PROPERTIES LINK_FLAGS ${GAIA_LINK_FLAGS})
  if(ENABLE_ASAN AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
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
  target_include_directories(gaia_db_jni PRIVATE ${PROJECT_SOURCE_DIR})
  target_include_directories(gaia_db_jni PRIVATE ${PROJECT_BINARY_DIR})
  target_include_directories(gaia_db_jni PRIVATE ${JNI_INCLUDE_DIRS})
  target_include_directories(gaia_db_jni PRIVATE ${GAIA_DB_CORE_INCLUDES})
  target_include_directories(gaia_db_jni PRIVATE ${GAIA_REPO}/production/db/inc/memory_manager)
  target_link_libraries(gaia_db_jni PRIVATE rt gaia_system gaia_db_client db_jni_headers)
  set_target_properties(gaia_db_jni PROPERTIES COMPILE_FLAGS
    "${GAIA_COMPILE_FLAGS} -Wno-deprecated-register -Wno-unused-parameter")
  set_target_properties(gaia_db_jni PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
  if(ENABLE_ASAN AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
    target_link_options(gaia_db_jni PRIVATE "-shared-libasan")
  endif()
endif()

set(GAIA_DB_CATALOG_TEST_PRIVATE_INCLUDES
  ${GAIA_INC}
  ${GAIA_REPO}/production/schemas/repository/inc
  ${GAIA_REPO}/production/db/core/inc
  ${GAIA_REPO}/production/db/inc/memory_manager
  ${FLATBUFFERS_INC}
)

add_library(gaia_db_catalog_test tests/db_catalog_test_base.cpp)
target_include_directories(gaia_db_catalog_test PRIVATE ${GAIA_DB_CATALOG_TEST_PRIVATE_INCLUDES})
target_link_libraries(gaia_db_catalog_test PRIVATE gaia_common gaia_db_client gaia_schema_loader gtest)
set_target_properties(gaia_db_catalog_test PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
set_target_properties(gaia_db_catalog_test PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")

add_gtest(test_rdb_object_converter tests/test_rdb_object_converter.cpp "${FLATBUFFERS_INC};${GEN_DIR};${GAIA_DB_CORE_TEST_INCLUDES}" "rocks_wrapper")
add_gtest(test_record_list tests/test_record_list.cpp "${GAIA_DB_CORE_TEST_INCLUDES}" "gaia_common;gaia_storage")
add_gtest(test_record_list_manager tests/test_record_list_manager.cpp "${GAIA_DB_CORE_TEST_INCLUDES}" "gaia_common;gaia_storage")
add_gtest(test_db_client tests/test_db_client.cpp "${GAIA_DB_CORE_TEST_INCLUDES}" "gaia_db_client;gaia_direct")
add_gtest(test_db_internals tests/test_db_internals.cpp "${GAIA_DB_CORE_TEST_INCLUDES}" "gaia_common;gaia_storage;gaia_db_client")
add_gtest(test_db_references tests/test_db_references.cpp "${GAIA_DB_CORE_TEST_INCLUDES}" "gaia_common;gaia_db_client;gaia_direct")

add_gtest(test_relationships
  tests/test_relationships.cpp
  "${GAIA_DB_CORE_TEST_INCLUDES};${GAIA_INC};${FLATBUFFERS_INC}"
  "gaia_common;gaia_db_client;gaia_catalog")

add_gtest(test_catalog_core
  tests/test_catalog_core.cpp
  "${FLATBUFFERS_INC};${GAIA_DB_CORE_TEST_INCLUDES};${GAIA_INC}"
  "gaia_common;gaia_db_client;gaia_catalog")
