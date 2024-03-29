# backward-cpp dependencies (one of several options):
# "libdwarf is a library to consume and produce DWARF debug information."
# (libelf and zlib are dependencies of libdwarf)
# NB: We need to force CMake to choose the static versions of these libraries,
# otherwise we would have to deploy additional shared libraries with our SDK.
find_library(LIBDWARF_LIBRARY NAMES libdwarf.a dwarf REQUIRED)
find_library(LIBELF_LIBRARY NAMES libelf.a elf REQUIRED)
find_library(ZLIB_LIBRARY NAMES libz.a zlib REQUIRED)

message(CHECK_START "Looking for backward")
find_package(Backward QUIET CONFIG)
if (Backward_FOUND)
  message(CHECK_PASS "found")
  add_library(backward INTERFACE)
  target_link_libraries(backward INTERFACE Backward::Backward)
  target_link_libraries(backward INTERFACE ${ZLIB_LIBRARY})
else ()
  message(CHECK_FAIL "not found")
  set(BACKWARD_VERSION "1.5")
  set(BACKWARD_ROOT ${DEPENDENCIES_INSTALL_PREFIX}/backward)
  add_library(backward INTERFACE)
  add_dependencies(backward backward_dependency)
  target_include_directories(backward SYSTEM INTERFACE ${BACKWARD_ROOT}/include)
  ExternalProject_Add(backward_dependency
    URL
      https://github.com/bombela/backward-cpp/archive/v${BACKWARD_VERSION}.tar.gz
    URL_HASH
      SHA256=faf7d4fe7ca65117ed4fe7be9bff9628927bd95b49f71df63d5f99af233d1915
    CMAKE_ARGS
      -G${CMAKE_GENERATOR}
      $<$<BOOL:${CCACHE_FOUND}>:-DCMAKE_CXX_COMPILER_LAUNCHER=ccache>
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
      -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
      -DBACKWARD_SHARED:BOOL=OFF
      -DBACKWARD_TESTS:BOOL=OFF
      -DCMAKE_INSTALL_PREFIX:PATH=${BACKWARD_ROOT}
    )
endif ()
