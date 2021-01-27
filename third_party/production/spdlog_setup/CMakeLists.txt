set(SPDLOG_SETUP_VERSION "0.3.2")
set(SPDLOG_SETUP_ROOT ${DEPENDENCIES_INSTALL_PREFIX}/spdlog_setup)
add_library(spdlog_setup INTERFACE)
add_dependencies(spdlog_setup spdlog_setup_dependency)
add_dependencies(spdlog_setup cpptoml)
add_dependencies(spdlog_setup spdlog)
target_include_directories(spdlog_setup INTERFACE ${SPDLOG_SETUP_ROOT}/include)
ExternalProject_Add(spdlog_setup_dependency
  URL
    https://github.com/guangie88/spdlog_setup/archive/v${SPDLOG_SETUP_VERSION}.tar.gz
  URL_HASH
    SHA256=08229cd17178f939d37de1f6d93d8983b6d1696e57c1359a967dddc65bc16a1e
  DEPENDS
    spdlog cpptoml
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_THREAD_LIBS_INIT="-lpthread"
    -DSPDLOG_SETUP_CPPTOML_EXTERNAL:BOOL=ON
    -DSPDLOG_SETUP_INCLUDE_UNIT_TESTS:BOOL=OFF
    -DCMAKE_PREFIX_PATH:PATH=${DEPENDENCIES_INSTALL_PREFIX}
    -DCMAKE_INSTALL_PREFIX:PATH=${SPDLOG_SETUP_ROOT}
)
