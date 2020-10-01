# backward-cpp dependencies (one of several options):
# "libdw provides access to DWARF debug information stored inside ELF files."
find_library(LIBDW_LIBRARY dw REQUIRED)

message(STATUS "Looking for backward dependency")
find_package(Backward QUIET CONFIG)
if (Backward_FOUND)
  message(STATUS "Looking for backward dependency -- found")
  add_library(backward INTERFACE)
  target_link_libraries(backward INTERFACE Backward::Backward)
else ()
  message(STATUS "Looking for backward dependency -- not found")
  set(BACKWARD_VERSION "1.5")
  set(BACKWARD_ROOT ${DEPENDENCIES_INSTALL_PREFIX}/backward)
  add_library(backward INTERFACE)
  add_dependencies(backward backward_dependency)
  target_include_directories(backward INTERFACE ${BACKWARD_ROOT}/include)
  target_link_directories(backward INTERFACE ${BACKWARD_ROOT}/lib)
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
