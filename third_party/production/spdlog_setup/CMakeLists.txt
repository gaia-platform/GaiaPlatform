message(STATUS "Looking for spdlog_setup dependency")
find_package(spdlog_setup QUIET CONFIG)
if (spdlog_setup_FOUND)
  message(STATUS "Looking for spdlog_setup dependency -- found")
  add_library(spdlog_setup INTERFACE)
else ()
  message(STATUS "Looking for spdlog_setup dependency -- not found")
  set(SPDLOG_SETUP_VERSION "0.3.0")
  set(SPDLOG_SETUP_ROOT ${DEPENDENCIES_INSTALL_PREFIX}/spdlog_setup)
  add_library(spdlog_setup INTERFACE)
  add_dependencies(spdlog_setup spdlog_setup_dependency)
  target_include_directories(spdlog_setup INTERFACE ${SPDLOG_SETUP_ROOT}/include)
  ExternalProject_Add(spdlog_setup_dependency
    URL
      https://github.com/guangie88/spdlog_setup/archive/v${SPDLOG_SETUP_VERSION}.tar.gz
    URL_HASH
      SHA256=d0cfa7d306ff65596e36be42e23c231a1b8f84e8fe339b94c15fca58c67516e3
    CMAKE_ARGS
      -DCMAKE_BUILD_TYPE=Release
      -DCMAKE_THREAD_LIBS_INIT="-lpthread"
      -DSPDLOG_SETUP_CPPTOML_EXTERNAL:BOOL=ON
      -DSPDLOG_SETUP_INCLUDE_UNIT_TESTS:BOOL=OFF
      -DCMAKE_INSTALL_PREFIX=${SPDLOG_SETUP_ROOT}
    )
endif ()
