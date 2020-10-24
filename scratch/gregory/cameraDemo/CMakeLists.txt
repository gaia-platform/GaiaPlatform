cmake_minimum_required(VERSION 3.11)

project(camera_demo VERSION 0.1.0)

#list(APPEND CMAKE_MODULE_PATH 
#  ${LLVM_LIB_PATH}/lib/cmake/clang
#  ${LLVM_LIB_PATH}/lib/cmake/llvm
# )
find_package(OpenCV 4.3.0 REQUIRED)

include(gaia)
set(GAIA_PROD_BUILD "${CMAKE_SOURCE_DIR}/../../../production/build")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")


set (CAMERA_DEMO_INCLUDES
  ${CMAKE_SOURCE_DIR}/../../../production/inc/public/rules
  ${CMAKE_SOURCE_DIR}/../../../production/inc/internal/rules
  ${CMAKE_SOURCE_DIR}/../../../production/inc/internal/common
  ${CMAKE_SOURCE_DIR}/../../../production/inc/public/common
  ${CMAKE_SOURCE_DIR}/../../../third_party/production/flatbuffers/include
  ${CMAKE_SOURCE_DIR}/../../../production/db/storage_engine/mock
  ${CMAKE_SOURCE_DIR}/../../../production/inc/public/direct_access
  ${CMAKE_SOURCE_DIR}/../../../production/inc/public/db
  ${CMAKE_SOURCE_DIR}/../../../production/inc/internal/db
  ${GAIA_PROD_BUILD}/rules/event_manager/generated
  ${CMAKE_SOURCE_DIR}
  ${OpenCV_INCLUDE_DIR}
  )

include_directories(${CAMERA_DEMO_INCLUDES})

set(CAMERA_DEMO_SRCS 
  "${CMAKE_SOURCE_DIR}/rules.cpp"
  "${CMAKE_SOURCE_DIR}/demo.cpp"
  "${CMAKE_SOURCE_DIR}/cv.cpp"
  )

add_library(gaia SHARED IMPORTED GLOBAL)
set_target_properties(gaia PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/system/libgaia.a)

add_library(gaia_se_client STATIC IMPORTED GLOBAL)
set_target_properties(gaia_se_client PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/db/storage_engine/libgaia_se_client.a)

add_library(gaia_rules SHARED IMPORTED GLOBAL)
set_target_properties(gaia_rules PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/rules/event_manager/libgaia_rules.a)

add_library(gaia_direct SHARED IMPORTED GLOBAL)
set_target_properties(gaia_direct PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/direct_access/libgaia_direct.a)

add_library(gaia_catalog SHARED IMPORTED GLOBAL)
set_target_properties(gaia_catalog PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/catalog/catalog_manager/libgaia_catalog.a)

add_library(gaia_parser SHARED IMPORTED GLOBAL)
set_target_properties(gaia_parser PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/catalog/parser/libgaia_parser.a)

add_library(flatbuffers STATIC IMPORTED GLOBAL)
set_target_properties(flatbuffers PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/flatbuffers/libflatbuffers.a)

add_library(gaia_types STATIC IMPORTED GLOBAL)
set_target_properties(gaia_types PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/db/types/libgaia_types.a)

add_library(gaia_common STATIC IMPORTED GLOBAL)
set_target_properties(gaia_common PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/common/libgaia_common.a)


add_executable(camera_demo ${CAMERA_DEMO_SRCS})


target_link_libraries(camera_demo
  PRIVATE
  gaia_rules
  gaia_se_client
  gaia
  gaia_types
  gaia_common
  gaia_direct
  gaia_catalog
  gaia_parser
  flatbuffers
  ${OpenCV_LIBS}
  explain
  m
  rt
  X11
)

