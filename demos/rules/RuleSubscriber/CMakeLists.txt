cmake_minimum_required(VERSION 3.11)

set(LLVM_LIB_PATH "/usr/lib/llvm-8")

list(APPEND CMAKE_MODULE_PATH 
  ${LLVM_LIB_PATH}/lib/cmake/clang
  ${LLVM_LIB_PATH}/lib/cmake/llvm
 )

set(CMAKE_CXX_FLAGS
      "-std=c++0x -Wall -Wextra -ggdb -Wno-unused-parameter -fno-rtti -DLLVM_DISABLE_ABI_BREAKING_CHECKS_ENFORCING=1")
set(CMAKE_EXE_LINKER_FLAGS "-ggdb -pthread")

add_executable(rule_subscriber RuleSubscriber.cpp)

set(search_paths
  ${LLVM_LIB_PATH}/lib/cmake/clang
  ${LLVM_LIB_PATH}/lib/cmake/llvm
)


find_package(LLVM REQUIRED CONFIG
  PATHS ${search_paths}
  NO_DEFAULT_PATH
)

find_package(Clang REQUIRED CONFIG
  PATHS ${search_paths}
  NO_DEFAULT_PATH
)

include_directories(
  ..
  ${LLVM_LIB_PATH}/include
  ${LLVM_LIB_PATH}/lib
)

target_link_libraries(rule_subscriber
  PRIVATE
  clangTooling
)

