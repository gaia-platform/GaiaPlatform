#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

project(flatbuffers)

set(FLATBUFFERS_VERSION "2.0.0")
set(FLATBUFFERS_ROOT ${DEPENDENCIES_INSTALL_PREFIX}/flatbuffers)

add_library(flatbuffers INTERFACE)
add_dependencies(flatbuffers flatbuffers_dependency)
target_include_directories(flatbuffers SYSTEM INTERFACE ${FLATBUFFERS_INC})
target_link_libraries(flatbuffers INTERFACE ${FLATBUFFERS_ROOT}/lib/libflatbuffers.a)

ExternalProject_Add(flatbuffers_dependency
  URL
    https://github.com/google/flatbuffers/archive/refs/tags/v${FLATBUFFERS_VERSION}.tar.gz
  URL_HASH
    SHA256=9ddb9031798f4f8754d00fca2f1a68ecf9d0f83dfac7239af1311e4fd9a565c4
  EXCLUDE_FROM_ALL
    TRUE
  PREFIX
    flatbuffers
  SOURCE_DIR
    src
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS}"
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    -DFLATBUFFERS_BUILD_TESTS=OFF
    -DFLATBUFFERS_INSTALL=ON
    -DCMAKE_INSTALL_PREFIX:PATH=${FLATBUFFERS_ROOT}
  BUILD_BYPRODUCTS
    ${FLATBUFFERS_INC}
    ${FLATBUFFERS}/src/idl_gen_cpp.cpp
    ${FLATBUFFERS_BIN}/flatc
)
