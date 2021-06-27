# backward-cpp dependencies (one of several options):
# "libdwarf is a library to consume and produce DWARF debug information."
# (libelf is a dependency of libdwarf)
find_library(LIBELF_LIBRARY elf REQUIRED)
find_library(LIBDWARF_LIBRARY dwarf REQUIRED)

message(CHECK_START "Looking for backward")
find_package(Backward QUIET CONFIG)
if (Backward_FOUND)
  message(CHECK_PASS "found")
  add_library(backward INTERFACE)
  target_link_libraries(backward INTERFACE Backward::Backward)
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
      -DBACKWARD_SHARED:BOOL=OFF
      -DBACKWARD_TESTS:BOOL=OFF
      -DCMAKE_INSTALL_PREFIX:PATH=${BACKWARD_ROOT}
    )
endif ()
