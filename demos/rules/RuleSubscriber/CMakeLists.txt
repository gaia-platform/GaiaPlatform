cmake_minimum_required(VERSION 3.11)

set(LLVM_LIB_PATH "/usr/lib/llvm-8")

list(APPEND CMAKE_MODULE_PATH 
  ${LLVM_LIB_PATH}/lib/cmake/clang
  ${LLVM_LIB_PATH}/lib/cmake/llvm
 )

include(AddLLVM)

macro(add_clang_executable name)
  add_llvm_executable( ${name} ${ARGN} )
  set_target_properties(${name} PROPERTIES FOLDER "Clang executables")
endmacro(add_clang_executable)


add_clang_executable(rule_subscriber RuleSubscriber.cpp)

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
  ${LLVM_LIB_PATH}/include/clang
  ${LLVM_LIB_PATH}/lib
)

target_link_libraries(rule_subscriber
  PRIVATE
  clangTooling
)
