message(CHECK_START "Looking for spdlog")
find_package(spdlog QUIET CONFIG)
if (spdlog_FOUND)
  message(CHECK_PASS "found")
  add_library(spdlog INTERFACE)
  target_link_libraries(spdlog INTERFACE spdlog::spdlog)
else ()
  message(CHECK_FAIL "not found")
  set(SPDLOG_VERSION "1.8.1")
  set(SPDLOG_ROOT ${DEPENDENCIES_INSTALL_PREFIX}/spdlog)
  add_library(spdlog INTERFACE)
  add_dependencies(spdlog fmt)
  add_dependencies(spdlog spdlog_dependency)
  target_include_directories(spdlog INTERFACE ${SPDLOG_ROOT}/include)
  target_link_libraries(spdlog INTERFACE ${SPDLOG_ROOT}/lib/libspdlog.a)
  ExternalProject_Add(spdlog_dependency
    URL
      https://github.com/gabime/spdlog/archive/v${SPDLOG_VERSION}.tar.gz
    URL_HASH
      SHA256=5197b3147cfcfaa67dd564db7b878e4a4b3d9f3443801722b3915cdeced656cb
    PATCH_COMMAND
      echo "set_property (TARGET spdlog PROPERTY POSITION_INDEPENDENT_CODE ON)" ">>" "${CMAKE_CURRENT_BINARY_DIR}/spdlog_dependency-prefix/src/spdlog_dependency/CMakeLists.txt"
    DEPENDS
      fmt
    CMAKE_ARGS
      -DCMAKE_BUILD_TYPE=Release
      -DSPDLOG_BUILD_BENCH:BOOL=OFF
      -DSPDLOG_BUILD_TESTS:BOOL=OFF
      -DSPDLOG_BUILD_EXAMPLE:BOOL=OFF
      -DSPDLOG_BUILD_SHARED:BOOL=OFF
      -DSPDLOG_INSTALL:BOOL=ON
      -DSPDLOG_FMT_EXTERNAL:BOOL=ON
      -DCMAKE_PREFIX_PATH:PATH=${DEPENDENCIES_INSTALL_PREFIX}
      -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON
      -DCMAKE_INSTALL_PREFIX:PATH=${SPDLOG_ROOT}
    )
endif ()
