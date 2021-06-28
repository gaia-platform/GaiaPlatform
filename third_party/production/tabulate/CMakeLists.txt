message(CHECK_START "Looking for tabulate")
find_package(tabulate QUIET CONFIG)
if (tabulate_FOUND)
  message(CHECK_PASS "found")
  add_library(tabulate INTERFACE)
else ()
  message(CHECK_FAIL "not found")
  set(TABULATE_VERSION "1.3")
  set(TABULATE_ROOT ${DEPENDENCIES_INSTALL_PREFIX}/tabulate)
  add_library(tabulate INTERFACE)
  add_dependencies(tabulate tabulate_dependency)
  target_include_directories(tabulate SYSTEM INTERFACE ${TABULATE_ROOT}/include)
  ExternalProject_Add(tabulate_dependency
    URL
      https://github.com/p-ranav/tabulate/archive/v${TABULATE_VERSION}.tar.gz
    URL_HASH
      SHA256=d1739e82f9f5acab084a5166a78714fde5198ce6b9448ec8ae8e403085048f7e
    CMAKE_ARGS
      -DUSE_CPP17:BOOL=ON
      -DCMAKE_INSTALL_PREFIX:PATH=${TABULATE_ROOT}
    )
endif ()
