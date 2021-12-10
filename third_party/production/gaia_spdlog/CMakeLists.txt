set(GAIA_SPDLOG_ROOT ${DEPENDENCIES_INSTALL_PREFIX}/gaia_spdlog)
add_library(gaia_spdlog INTERFACE)
add_dependencies(gaia_spdlog spdlog_dependency)
target_include_directories(gaia_spdlog SYSTEM INTERFACE ${GAIA_SPDLOG_ROOT}/include)
target_link_libraries(gaia_spdlog INTERFACE ${GAIA_SPDLOG_ROOT}/lib/libgaia_spdlog.a)
# Uses the local gaia_spdlog sources to build the static library.
# REVIEW: The build fails for some reason unless we specify -DCMAKE_BUILD_TYPE=Release.
ExternalProject_Add(spdlog_dependency
  SOURCE_DIR
    ${GAIA_REPO}/third_party/bundle/gaia_spdlog
  INSTALL_DIR ${DEPENDENCIES_INSTALL_PREFIX}/gaia_spdlog
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DGAIA_SPDLOG_BUILD_BENCH:BOOL=OFF
    -DGAIA_SPDLOG_BUILD_TESTS:BOOL=OFF
    -DGAIA_SPDLOG_BUILD_SHARED:BOOL=OFF
    -DGAIA_SPDLOG_INSTALL:BOOL=ON
    -DGAIA_SPDLOG_FMT_EXTERNAL:BOOL=OFF
    -DCMAKE_PREFIX_PATH:PATH=${DEPENDENCIES_INSTALL_PREFIX}
    -DCMAKE_INSTALL_PREFIX:PATH=${GAIA_SPDLOG_ROOT}
  BUILD_BYPRODUCTS ${GAIA_SPDLOG_ROOT}/lib/libgaia_spdlog.a
)
