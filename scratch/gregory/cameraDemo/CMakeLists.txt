cmake_minimum_required(VERSION 3.11)

project(camera_demo VERSION 0.1.0)

#list(APPEND CMAKE_MODULE_PATH 
#  ${LLVM_LIB_PATH}/lib/cmake/clang
#  ${LLVM_LIB_PATH}/lib/cmake/llvm
# )
find_package(OpenCV 4.3.0 REQUIRED)

include(gaia)
set(GAIA_PROD_BUILD "${CMAKE_SOURCE_DIR}/../../../production/build")
set(CMAKE_CXX_FLAGS
      "-std=c++0x -Wall -Wextra -ggdb -Wno-unused-parameter -ggdb")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ggdb -pthread")


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

# GAIA_DEMO_BUILD used by gaia_gen_rule_subscriptions.

set(CAMERA_DEMO_SRCS 
  "${CMAKE_SOURCE_DIR}/rules.cpp"
  "${CMAKE_SOURCE_DIR}/demo.cpp"
  "${CMAKE_SOURCE_DIR}/cv.cpp"
  )
message (STATUS "${GAIA_PROD_BUILD}")
add_library(gaia SHARED IMPORTED GLOBAL)
set_target_properties(gaia PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/system/libgaia.a)

add_library(gaia_semock SHARED IMPORTED GLOBAL)
set_target_properties(gaia_semock PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/db/storage_engine/libgaia_semock.a)

add_library(gaia_rules SHARED IMPORTED GLOBAL)
set_target_properties(gaia_rules PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/rules/event_manager/libgaia_rules.a)

add_library(gaia_direct SHARED IMPORTED GLOBAL)
set_target_properties(gaia_direct PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/direct_access/libgaia_direct.a)

add_executable(camera_demo ${CAMERA_DEMO_SRCS})


target_link_libraries(camera_demo
  PRIVATE
  gaia_rules
  gaia_semock
  gaia
  gaia_direct
  ${OpenCV_LIBS}
  explain
  m
  rt
  X11
)

