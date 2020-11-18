#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(storage_engine)

# We need pthreads support.
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

set(GAIA_STORAGE_ENGINE_PUBLIC_INCLUDES
  "${GAIA_INC}"
  "${GAIA_REPO}/production/db/inc/types"
  # This should be a public include since it exposes the gaia_ptr interface,
  # which is consumed by internal clients.
  "${GAIA_INTERNAL_INC}/db"
)

message(VERBOSE "GAIA_STORAGE_ENGINE_PUBLIC_INCLUDES=${GAIA_STORAGE_ENGINE_PUBLIC_INCLUDES}")

set(GAIA_STORAGE_ENGINE_PRIVATE_INCLUDES
  "${GAIA_INTERNAL_INC}/common"
  "${GAIA_DB_INC}/storage_engine"
  "${GAIA_DB_INC}/storage_engine/inc"
  "${ROCKSDB_INC}"
  "${PROJECT_SOURCE_DIR}/inc"
)

message(VERBOSE "GAIA_STORAGE_ENGINE_PRIVATE_INCLUDES=${GAIA_STORAGE_ENGINE_PRIVATE_INCLUDES}")

###############################################
# Generate flatbuffers headers.
###############################################
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

# The names of the following source files should be unique because they'll be used as targets.
set(FBS_SOURCES
  "${PROJECT_SOURCE_DIR}/src/flatbuffers/messages.fbs"
  "${PROJECT_SOURCE_DIR}/src/flatbuffers/gaia_table.fbs"
  "${PROJECT_SOURCE_DIR}/src/flatbuffers/gaia_field.fbs")

foreach(FBS_SOURCE ${FBS_SOURCES})
  get_filename_component(FBS_SOURCE_FILENAME ${FBS_SOURCE} NAME)
  string(REGEX REPLACE "\\.fbs$" "_generated.h" GEN_HEADER ${FBS_SOURCE_FILENAME})

  add_custom_command(
    OUTPUT "${GEN_DIR}/${GEN_HEADER}"
    COMMAND cmake -E make_directory "${GEN_DIR}"
    COMMAND flatc --cpp --scoped-enums -o "${GEN_DIR}" "${FBS_SOURCE}"
    DEPENDS "${FBS_SOURCE}"
    DEPENDS ${CMAKE_BINARY_DIR}/flatbuffers/flatc
    COMMENT "Completed flatbuffers compilation of ${FBS_SOURCE}!"
    VERBATIM)

  add_custom_target(${FBS_SOURCE_FILENAME} ALL DEPENDS "${GEN_DIR}/${GEN_HEADER}")
endforeach(FBS_SOURCE)

###############################################
# Our main target.
###############################################
add_library(gaia_storage SHARED
  src/locator_allocator.cpp
  src/record_list.cpp)
target_include_directories(gaia_storage PUBLIC ${GAIA_STORAGE_ENGINE_PUBLIC_INCLUDES})
target_include_directories(gaia_storage PRIVATE ${GAIA_STORAGE_ENGINE_PRIVATE_INCLUDES})
set_target_properties(gaia_storage PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
target_link_libraries(gaia_storage PRIVATE gaia_common)

###############################################
# Storage engine-related code begins here.
###############################################
add_library(gaia_se_client STATIC
  src/catalog_core.cpp
  src/gaia_db.cpp
  src/gaia_ptr.cpp
  src/payload_diff.cpp
  src/se_client.cpp
  src/se_shared_data_client.cpp
  src/type_metadata.cpp
  src/type_id_record_id_cache.cpp)
add_dependencies(gaia_se_client "messages.fbs" "gaia_table.fbs" "gaia_field.fbs")
target_include_directories(gaia_se_client PUBLIC ${GAIA_STORAGE_ENGINE_PUBLIC_INCLUDES})
target_include_directories(gaia_se_client PRIVATE ${GAIA_STORAGE_ENGINE_PRIVATE_INCLUDES})

# For referential integrity check to read catalog.
# TODO: Switch to use catalog_core and remove the dependency on catalog headers and gaia_direct.
target_include_directories(gaia_se_client PRIVATE
  "${GAIA_INC}"
  "${GAIA_INTERNAL_INC}/catalog")

# For flatbuffers generated code.
target_include_directories(gaia_se_client SYSTEM PRIVATE "${FLATBUFFERS_INC}")
target_include_directories(gaia_se_client SYSTEM PRIVATE "${GEN_DIR}")
# Suppress spurious warnings about zero-initialized structs.
set_target_properties(gaia_se_client PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -Wno-missing-field-initializers")
target_link_libraries(gaia_se_client PUBLIC gaia_common)
target_link_libraries(gaia_se_client PRIVATE explain gaia_payload_types)
# This static library must be compatible with linking in a shared library.
set_property (TARGET gaia_se_client PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(rocks_wrapper STATIC
  src/rdb_internal.cpp
  src/rdb_object_converter.cpp
  src/persistent_store_manager.cpp)
target_include_directories(rocks_wrapper PRIVATE
  "${GAIA_STORAGE_ENGINE_PUBLIC_INCLUDES}"
  "${GAIA_STORAGE_ENGINE_PRIVATE_INCLUDES}")
target_include_directories(rocks_wrapper SYSTEM PRIVATE "${FLATBUFFERS_INC}")
target_include_directories(rocks_wrapper SYSTEM PRIVATE "${GEN_DIR}")
# The client is required to create gaia objects on recovery.
target_link_libraries(rocks_wrapper rocksdb)
set_target_properties(rocks_wrapper PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -Wno-missing-field-initializers")

set(GAIA_SE_SERVER_SOURCES
  src/se_server.cpp
  src/se_shared_data_server.cpp
  src/se_server_exec.cpp
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  list(APPEND GAIA_SE_SERVER_SOURCES src/backward.cpp)
endif()

add_executable(gaia_se_server ${GAIA_SE_SERVER_SOURCES})
add_dependencies(gaia_se_server "messages.fbs")
target_include_directories(gaia_se_server PRIVATE
  "${GAIA_STORAGE_ENGINE_PUBLIC_INCLUDES}"
  "${GAIA_STORAGE_ENGINE_PRIVATE_INCLUDES}")

# For flatbuffers generated code.
target_include_directories(gaia_se_server SYSTEM PRIVATE "${FLATBUFFERS_INC}")
target_include_directories(gaia_se_server SYSTEM PRIVATE "${GEN_DIR}")

# Suppress spurious warnings about zero-initialized structs.
set_target_properties(gaia_se_server PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -Wno-missing-field-initializers")
set_target_properties(gaia_se_server PROPERTIES LINK_FLAGS ${GAIA_LINK_FLAGS})
target_link_libraries(gaia_se_server PRIVATE gaia_common Threads::Threads explain rocks_wrapper)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_link_libraries(gaia_se_server PUBLIC backward)
endif()
install(TARGETS gaia_se_server DESTINATION "bin")

set(GAIA_STORAGE_ENGINE_TEST_INCLUDES
  ${GAIA_STORAGE_ENGINE_PUBLIC_INCLUDES}
  ${GAIA_STORAGE_ENGINE_PRIVATE_INCLUDES}
)

###############################################
# PyBind11 wrapper for storage engine.
###############################################
find_package(pybind11 REQUIRED)

if(Python3_FOUND AND pybind11_FOUND)
  pybind11_add_module(gaia_se src/se_pybind_wrapper.cpp)
  target_include_directories(gaia_se PRIVATE ${PROJECT_SOURCE_DIR})
  target_include_directories(gaia_se PRIVATE
  "${GAIA_STORAGE_ENGINE_PUBLIC_INCLUDES}"
  "${GAIA_STORAGE_ENGINE_PRIVATE_INCLUDES}")
  target_include_directories(gaia_se SYSTEM PRIVATE "${FLATBUFFERS_INC}")
  target_include_directories(gaia_se SYSTEM PRIVATE "${GEN_DIR}")
  target_link_libraries(gaia_se PRIVATE pybind11::module rt gaia_se_client)
  # See https://github.com/pybind/pybind11/issues/1604 for inclusion of -fsized-deallocation for C++17
  set_target_properties(gaia_se PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -Wno-deprecated-register -Wno-unused-parameter -fsized-deallocation")
  set_target_properties(gaia_se PROPERTIES LINK_FLAGS ${GAIA_LINK_FLAGS})

  # FIXME: fix this test in https://gaiaplatform.atlassian.net/browse/GAIAPLAT-260.
  # Python wrapper unit test.
  # configure_file(tests/test_storage_engine.py . COPYONLY)
  # add_test(NAME test_se_client_pybind COMMAND ${Python3_EXECUTABLE} test_storage_engine.py)
  # set_tests_properties(test_se_client_pybind PROPERTIES PASS_REGULAR_EXPRESSION ${TEST_SUCCESS})
endif()

###############################################
# JNI wrapper for storage engine.
###############################################
if(JAVA_FOUND AND JNI_FOUND AND EXISTS "${GREMLIN_CONSOLE_PATH}")
  # Configure build of native C++ wrapper library.
  add_library(jni_se_client SHARED src/se_jni_wrapper.cpp)
  target_include_directories(jni_se_client PRIVATE ${PROJECT_SOURCE_DIR})
  target_include_directories(jni_se_client PRIVATE ${PROJECT_BINARY_DIR})
  target_include_directories(jni_se_client PRIVATE ${JNI_INCLUDE_DIRS})
  target_include_directories(jni_se_client PRIVATE ${GAIA_STORAGE_ENGINE_INCLUDES})
  target_link_libraries(jni_se_client PRIVATE
    rt
    gaia_se_client
    se_jni_headers)
  set_target_properties(jni_se_client PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -Wno-deprecated-register -Wno-unused-parameter")
  set_target_properties(jni_se_client PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
endif()

add_gtest(test_rdb_object_converter "tests/test_rdb_object_converter.cpp" "${FLATBUFFERS_INC};${GEN_DIR};${GAIA_STORAGE_ENGINE_TEST_INCLUDES}" "rocks_wrapper")
add_gtest(test_record_list tests/test_record_list.cpp "${GAIA_STORAGE_ENGINE_TEST_INCLUDES}" "gaia_common;gaia_storage")
add_gtest(test_se_client "tests/test_se_client.cpp" "${GAIA_STORAGE_ENGINE_TEST_INCLUDES}" "gaia_se_client;gaia_direct")
add_gtest(test_locator_allocator tests/test_locator_allocator.cpp "${GAIA_STORAGE_ENGINE_TEST_INCLUDES}" "gaia_common;gaia_storage")
add_gtest(test_relationships tests/test_relationships.cpp "${GAIA_STORAGE_ENGINE_TEST_INCLUDES}" "gaia_common;gaia_se_client;gaia_direct")
add_gtest(test_se_references tests/test_se_references.cpp "${GAIA_STORAGE_ENGINE_TEST_INCLUDES}" "gaia_common;gaia_se_client;gaia_direct")
