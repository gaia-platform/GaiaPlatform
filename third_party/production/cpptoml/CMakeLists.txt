message(CHECK_START "Looking for cpptoml")
# Use 'find_path' instead of 'find_package' to detect cpptoml because its cmake config is broken.
# It exports the 'cpptoml' INTERFACE library and sets the wrong linkage (-lcpptoml).
find_path(CPPTOML_INCLUDE "cpptoml.h")
if (CPPTOML_INCLUDE)
  message(CHECK_PASS "found")
  add_library(cpptoml INTERFACE)
  target_include_directories(cpptoml INTERFACE ${CPPTOML_INCLUDE})
else ()
  message(CHECK_FAIL "not found")
  set(CPPTOML_VERSION "0.1.1")
  set(CPPTOML_ROOT ${DEPENDENCIES_INSTALL_PREFIX}/cpptoml)
  add_library(cpptoml INTERFACE)
  set_property (TARGET cpptoml PROPERTY POSITION_INDEPENDENT_CODE ON)
  add_dependencies(cpptoml cpptoml_dependency)
  target_include_directories(cpptoml INTERFACE ${CPPTOML_ROOT}/include)
  ExternalProject_Add(cpptoml_dependency
    URL
      https://github.com/skystrife/cpptoml/archive/v${CPPTOML_VERSION}.tar.gz
    URL_HASH
      SHA256=23af72468cfd4040984d46a0dd2a609538579c78ddc429d6b8fd7a10a6e24403
    EXCLUDE_FROM_ALL
      TRUE
    CMAKE_ARGS
      -DCMAKE_BUILD_TYPE=None
      -DENABLE_LIBCXX:BOOL=OFF
      -DCPPTOML_BUILD_EXAMPLES:BOOL=OFF
      -DCMAKE_EXPORT_NO_PACKAGE_REGISTRY:BOOL=ON
      -DCMAKE_INSTALL_PREFIX:PATH=${CPPTOML_ROOT}
      -DBUILD_SHARED_LIBS:BOOL=OFF
    )
endif ()
