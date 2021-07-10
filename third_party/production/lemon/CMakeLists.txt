set(LEMON_ROOT ${DEPENDENCIES_INSTALL_PREFIX}/lemon)
add_library(lemon INTERFACE)
add_dependencies(lemon lemon_dependency)
target_include_directories(lemon SYSTEM INTERFACE ${LEMON_ROOT}/include)
target_link_libraries(lemon INTERFACE ${LEMON_ROOT}/lib/libemon.a)
# Uses the local lemon sources to build the static library.
ExternalProject_Add(lemon_dependency
  URL
    http://lemon.cs.elte.hu/pub/sources/lemon-1.3.1.zip
  URL_HASH
    SHA256=2222c2b2e58f556d6d53117dfbd9c5b4fc1fceb41a6eaca3c9a3e58b66f9e46f
  EXCLUDE_FROM_ALL
    TRUE
  INSTALL_DIR ${DEPENDENCIES_INSTALL_PREFIX}/lemon
  CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_PREFIX_PATH:PATH=${DEPENDENCIES_INSTALL_PREFIX}
    -DCMAKE_INSTALL_PREFIX:PATH=${LEMON_ROOT}
  BUILD_BYPRODUCTS ${LEMON_ROOT}/lib/libemon.a
)
