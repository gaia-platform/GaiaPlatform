#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.12)

# Set the project name.
project(storage_engine)

# We need pthreads support.
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

set(GAIA_STORAGE_ENGINE_PUBLIC_INCLUDES
  "${GAIA_INC}/public/common"
  "${GAIA_INC}/public/db"
  # This should be a public include since it exposes the gaia_ptr interface,
  # which is consumed by internal clients.
  "${GAIA_INC}/internal/db"
)
message(STATUS "GAIA_STORAGE_ENGINE_PUBLIC_INCLUDES=${GAIA_STORAGE_ENGINE_PUBLIC_INCLUDES}")

set(GAIA_STORAGE_ENGINE_PRIVATE_INCLUDES
  "${GAIA_INC}/internal/common"
  "${GAIA_DB_INC}/storage_engine"
  "${ROCKSDB_INC}"
  "${PROJECT_SOURCE_DIR}/mock"
)
message(STATUS "GAIA_STORAGE_ENGINE_PRIVATE_INCLUDES=${GAIA_STORAGE_ENGINE_PRIVATE_INCLUDES}")

# Generate flatbuffers headers.
find_program(FLATC flatc)
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_SOURCES
    "${PROJECT_SOURCE_DIR}/mock/flatbuffers/messages.fbs")
add_custom_target("${PROJECT_NAME}-gen_fbs" ALL)
foreach(fbs ${FBS_SOURCES})
  add_custom_command(
    TARGET "${PROJECT_NAME}-gen_fbs"
    COMMAND cmake -E make_directory "${GEN_DIR}"
    COMMAND "${FLATC}" --cpp --scoped-enums -o "${GEN_DIR}" "${fbs}" DEPENDS "${fbs}"
    VERBATIM)
endforeach(fbs)

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
# Storage engine mock-related code begins here.
###############################################
add_library(gaia_semock STATIC
  mock/gaia_db.cpp
  mock/gaia_ptr.cpp
  mock/storage_engine.cpp
  mock/storage_engine_client.cpp
  mock/rdb_object_converter.cpp)

add_dependencies(gaia_semock "${PROJECT_NAME}-gen_fbs")
target_include_directories(gaia_semock PUBLIC ${GAIA_STORAGE_ENGINE_PUBLIC_INCLUDES})
target_include_directories(gaia_semock PRIVATE ${GAIA_STORAGE_ENGINE_PRIVATE_INCLUDES})
# for flatbuffers generated code
target_include_directories(gaia_semock SYSTEM PRIVATE "${FLATBUFFERS_INC}")
target_include_directories(gaia_semock SYSTEM PRIVATE "${GEN_DIR}")
# suppress spurious warnings about zero-initialized structs
set_target_properties(gaia_semock PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -Wno-missing-field-initializers")
target_link_libraries(gaia_semock PUBLIC gaia_common)
target_link_libraries(gaia_semock PRIVATE explain)
# This static library must be compatible with linking in a shared library.
set_property (TARGET gaia_semock PROPERTY POSITION_INDEPENDENT_CODE ON)

add_executable(gaia_semock_server
  mock/storage_engine.cpp
  mock/storage_engine_server.cpp
  mock/storage_engine_server_exec.cpp)
add_dependencies(gaia_semock_server "${PROJECT_NAME}-gen_fbs")
target_include_directories(gaia_semock_server PRIVATE
  "${GAIA_STORAGE_ENGINE_PUBLIC_INCLUDES}"
  "${GAIA_STORAGE_ENGINE_PRIVATE_INCLUDES}"
)
# for flatbuffers generated code
target_include_directories(gaia_semock_server SYSTEM PRIVATE "${FLATBUFFERS_INC}")
target_include_directories(gaia_semock_server SYSTEM PRIVATE "${GEN_DIR}")
# suppress spurious warnings about zero-initialized structs
set_target_properties(gaia_semock_server PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -Wno-missing-field-initializers")
target_link_libraries(gaia_semock_server PRIVATE gaia_common Threads::Threads explain)
install(TARGETS gaia_semock_server DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")

# PyBind11 wrapper for storage engine mock.
find_package(pybind11 REQUIRED)
if (Python3_FOUND AND pybind11_FOUND)
  pybind11_add_module(se_mock mock/storage_engine_pybind_wrapper.cpp)
  target_include_directories(se_mock PRIVATE ${PROJECT_SOURCE_DIR})
  target_include_directories(se_mock PRIVATE ${GAIA_STORAGE_ENGINE_INCLUDES})
  target_link_libraries(se_mock PRIVATE pybind11::module rt gaia_semock)
  set_target_properties(se_mock PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -Wno-deprecated-register -Wno-unused-parameter")
  set_target_properties(se_mock PROPERTIES LINK_FLAGS ${GAIA_LINK_FLAGS})

  # FIXME: fix this test in https://gaiaplatform.atlassian.net/browse/GAIAPLAT-260.
  # Python wrapper unit test.
  # configure_file(tests/test_storage_engine_mock.py . COPYONLY)
  # add_test(NAME test_se_mock_pybind COMMAND ${Python3_EXECUTABLE} test_storage_engine_mock.py)
  # set_tests_properties(test_se_mock_pybind PROPERTIES PASS_REGULAR_EXPRESSION ${TEST_SUCCESS})
endif()

# JNI wrapper for storage engine mock.
if (JAVA_FOUND AND JNI_FOUND AND EXISTS "${GREMLIN_CONSOLE_PATH}")
  # Configure build of native C++ wrapper library.
  add_library(jni_se_mock SHARED mock/storage_engine_jni_wrapper.cpp)
  target_include_directories(jni_se_mock PRIVATE ${PROJECT_SOURCE_DIR})
  target_include_directories(jni_se_mock PRIVATE ${PROJECT_BINARY_DIR})
  target_include_directories(jni_se_mock PRIVATE ${JNI_INCLUDE_DIRS})
  target_include_directories(jni_se_mock PRIVATE ${GAIA_STORAGE_ENGINE_INCLUDES})
  target_link_libraries(jni_se_mock PRIVATE
    rt
    gaia_semock
    storage_engine_mock_jni_headers)
  set_target_properties(jni_se_mock PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS} -Wno-deprecated-register -Wno-unused-parameter")
  set_target_properties(jni_se_mock PROPERTIES LINK_FLAGS "${GAIA_LINK_FLAGS}")
endif()

set(GAIA_STORAGE_ENGINE_TEST_INCLUDES
  ${GAIA_STORAGE_ENGINE_PUBLIC_INCLUDES}
  ${GAIA_STORAGE_ENGINE_PRIVATE_INCLUDES}
)

add_gtest(test_rdb_object_converter "tests/test_rdb_object_converter.cpp" "${GAIA_STORAGE_ENGINE_TEST_INCLUDES}" "gaia_semock")
add_gtest(test_record_list tests/test_record_list.cpp "${GAIA_STORAGE_ENGINE_TEST_INCLUDES}" "gaia_common;gaia_storage")
add_gtest(test_storage_engine_client "tests/test_storage_engine_client.cpp" "${GAIA_STORAGE_ENGINE_TEST_INCLUDES}" "gaia_semock")
add_gtest(test_locator_allocator tests/test_locator_allocator.cpp "${GAIA_STORAGE_ENGINE_TEST_INCLUDES}" "gaia_common;gaia_storage")
