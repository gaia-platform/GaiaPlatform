cmake_minimum_required(VERSION 3.11)



#list(APPEND CMAKE_MODULE_PATH 
#  ${LLVM_LIB_PATH}/lib/cmake/clang
#  ${LLVM_LIB_PATH}/lib/cmake/llvm
# )

set(camera_demo_SRCS
  
  ../ruleInit.cpp
  ../rules.cpp
  ../demo.cpp
  ../cv.cpp
)

find_package(OpenCV 4.3.0 REQUIRED)

set(CMAKE_CXX_FLAGS
      "-std=c++0x -Wall -Wextra -ggdb -Wno-unused-parameter -ggdb")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ggdb -pthread")

add_executable(camera_demo ${camera_demo_SRCS})

target_link_directories(camera_demo
  PRIVATE
  /usr/local/lib
  ${CMAKE_BINARY_DIR}/../../../../production/build/direct_access
  ${CMAKE_BINARY_DIR}/../../../../production/build/rules/event_manager
  ${CMAKE_BINARY_DIR}/../../../../production/build/db/storage_engine
  ${CMAKE_BINARY_DIR}/../../../../production/build/system
)

include_directories(
  ..
  ${CMAKE_BINARY_DIR}/../../../../production/inc/public/rules
  ${CMAKE_BINARY_DIR}/../../../../production/inc/internal/common 
  ${CMAKE_BINARY_DIR}/../../../../production/inc/public/common 
  ${CMAKE_BINARY_DIR}/../../../../third_party/production/flatbuffers/include 
  ${CMAKE_BINARY_DIR}/../../../../production/db/storage_engine/mock
  ${CMAKE_BINARY_DIR}/../../../../production/build/rules/event_manager/generated
  ${OpenCV_INCLUDE_DIR}
)

target_link_libraries(camera_demo
  PRIVATE
  gaia_rules
  gaia_semock
  gaia
  gaia_direct
  ${OpenCV_LIBS}
  m
  rt
)

