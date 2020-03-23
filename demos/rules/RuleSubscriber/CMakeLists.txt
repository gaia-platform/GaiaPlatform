cmake_minimum_required(VERSION 3.10)


list(APPEND CMAKE_MODULE_PATH 
  "/usr/lib/llvm-8/lib/cmake/clang"
  "/usr/lib/llvm-8/lib/cmake/llvm"
 )

include(AddLLVM)
macro(add_clang_executable name)
  add_llvm_executable( ${name} ${ARGN} )
  set_target_properties(${name} PROPERTIES FOLDER "Clang executables")
endmacro(add_clang_executable)


add_clang_executable(RuleSubscriber RuleSubscriber.cpp)

set(search_paths
  /usr/lib/llvm-8/lib/cmake/clang
  /usr/lib/llvm-8/lib/cmake/llvm
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
  ${/usr/lib/llvm-8/include/clang}
  ${/usr/lib/llvm-8/lib}
)

llvm_map_components_to_libnames(LLVM_LIBS
  Core
  ExecutionEngine
  MC
  MCJIT
  Object
  OrcJit
  Option
  RuntimeDyld
  Support
  native
)

target_link_libraries(RuleSubscriber
  PRIVATE
  clangBasic
  clangCodeGen
  clangDriver
  clangFrontend
  clangSerialization
  clangTooling
  ${LLVM_LIBS}
)
