message(CHECK_START "Looking for fmt")
find_package(fmt QUIET CONFIG)
if (fmt_FOUND)
  message(CHECK_PASS "found")
  add_library(fmt INTERFACE)
  target_link_libraries(fmt INTERFACE fmt::fmt)
else ()
  message(CHECK_FAIL "not found")
  set(FMT_VERSION "7.0.3")
  set(FMT_ROOT ${DEPENDENCIES_INSTALL_PREFIX}/fmt)
  add_library(fmt INTERFACE)
  add_dependencies(fmt fmt_dependency)
  set_property (TARGET fmt PROPERTY POSITION_INDEPENDENT_CODE ON)
  target_include_directories(fmt INTERFACE ${FMT_ROOT}/include)
  target_link_libraries(fmt INTERFACE ${FMT_ROOT}/lib/libfmt.a)
  ExternalProject_Add(fmt_dependency
    URL
      https://github.com/fmtlib/fmt/archive/${FMT_VERSION}.tar.gz
    URL_HASH
      SHA256=b4b51bc16288e2281cddc59c28f0b4f84fed58d016fb038273a09f05f8473297
    CMAKE_ARGS
      -DFMT_INSTALL:BOOL=ON
      -DFMT_TEST:BOOL=OFF
      -DFMT_FUZZ:BOOL=OFF
      -DFMT_DOC:BOOL=OFF
      -DBUILD_SHARED_LIBS:BOOL=OFF
      -DCMAKE_INSTALL_PREFIX:PATH=${FMT_ROOT}
    )
endif ()
