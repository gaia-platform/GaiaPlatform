#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

project(sdk)

include(gaia_internal)

file(WRITE ${CMAKE_BINARY_DIR}/sdk_dummy.c "")

# TODO: Replace dummy source with object libraries.
add_library(gaia SHARED ${CMAKE_BINARY_DIR}/sdk_dummy.c)
set(SDK_LINK_LIBRARIES
  -Wl,--whole-archive
  gaia_system
  -Wl,--no-whole-archive
)
target_compile_options(gaia PRIVATE -Qunused-arguments)
target_link_libraries(gaia PRIVATE
  ${SDK_LINK_LIBRARIES}
)

install(TARGETS gaia DESTINATION ${CMAKE_INSTALL_LIBDIR})

#
# Ensure this test only includes public headers and links to the gaia shared library
# Emulate what an SDK user does as closely as possible minus gaiat functionality for
# a production only (non-gaia release) build.
#
set(SDK_TEST_INCLUDES
  ${GAIA_INC}
  ${GAIA_PROD_BUILD}/deps/gaia_spdlog/include
  ${GAIA_PROD_BUILD}/gaia_generated/addr_book
)

configure_file("${GAIA_CONFIG}" "${PROJECT_BINARY_DIR}/gaia.conf")
configure_file("${GAIA_LOG_CONFIG}" "${PROJECT_BINARY_DIR}/gaia_log.conf")
configure_file("${GAIA_REPO}/production/schemas/test/addr_book/addr_book.ddl" "${PROJECT_BINARY_DIR}/addr_book.ddl") 

add_gtest(test_sdk "tests/test_sdk.cpp;${GAIA_PROD_BUILD}/gaia_generated/addr_book/gaia_addr_book.cpp" "${SDK_TEST_INCLUDES}" "rt;gaia")
add_gtest(test_sdk_no_init_rules "tests/test_sdk_no_init_rules.cpp" "${SDK_TEST_INCLUDES}" "rt;gaia;dac_addr_book")

# Set CPack variables required for building a DEB/RPM package.
#
# See:
# https://cmake.org/cmake/help/v3.17/module/CPack.html#variables-common-to-all-cpack-generators
# https://cmake.org/cmake/help/v3.17/cpack_gen/deb.html#variables-specific-to-cpack-debian-deb-generator
# https://cmake.org/cmake/help/v3.17/cpack_gen/rpm.html#variables-specific-to-cpack-rpm-generator
#
set(CPACK_PACKAGE_CONTACT "info@gaiaplatform.io")
set(CPACK_PACKAGE_VENDOR "Gaia Platform")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://www.gaiaplatform.io/")
set(CPACK_PACKAGE_NAME "gaia")
set(CPACK_PACKAGE_DESCRIPTION "Gaia Platform SDK")
set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/gaia")
set(CPACK_PACKAGE_VERSION_MAJOR "${production_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${production_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${production_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

# DEB specific section.
#
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE amd64)
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION ${CPACK_PACKAGE_DESCRIPTION})
set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${CPACK_PACKAGE_CONTACT})
set(CPACK_DEBIAN_FILE_NAME ${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  # We only link libc++ in Debug builds.
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "clang-10,lld-10,libc++-10-dev,libc++abi-10-dev")
  # REVIEW (GAIAPLAT-1474): When CPack correctly applies
  # CPACK_DEBIAN_PACKAGE_RECOMMENDS, then uncomment the following 2 lines and
  # remove the preceding definition of CPACK_DEBIAN_PACKAGE_DEPENDS.
  # set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc++1-10,libc++abi1-10")
  # set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "clang-10,lld-10,libc++-10-dev,libc++abi-10-dev")
  # REVIEW: CMake ideally would be an included dependency, but Kitware does not
  # version their CMake debian packages, so it is impossible to avoid clobbering
  # newer versions of CMake if we include it in CPACK_DEBIAN_PACKAGE_DEPENDS.
  # Once CPack respects CPACK_DEBIAN_PACKAGE_RECOMMENDS and
  # CPACK_DEBIAN_PACKAGE_SUGGESTS, we could consider including CMake as a
  # "Recommends" or "Suggests" dependency.
else()
  # We use libstdc++ in Release builds for compatibility with other software
  # built with libstdc++ (which is the default stdlib in both gcc and clang).
  # GCC 9 is installed by default on Ubuntu 20.04 with the build-essential
  # package, so we try to be compatible with that.
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "clang-10,lld-10,libstdc++-9-dev")
  # REVIEW (GAIAPLAT-1474): When CPack correctly applies
  # CPACK_DEBIAN_PACKAGE_RECOMMENDS, then uncomment the following 2 lines and
  # remove the preceding definition of CPACK_DEBIAN_PACKAGE_DEPENDS.
  # set(CPACK_DEBIAN_PACKAGE_DEPENDS "libstdc++6")
  # set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "clang-10,lld-10,libstdc++-9-dev")
endif()

set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
set(CPACK_DEBIAN_PACKAGE_SECTION "custom")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")

# RPM specific section.
#
set(CPACK_RPM_PACKAGE_ARCHITECTURE amd64)
set(CPACK_RPM_PACKAGE_DESCRIPTION ${CPACK_PACKAGE_DESCRIPTION})
set(CPACK_RPM_FILE_NAME ${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.rpm)
set(CPACK_RPM_PACKAGE_REQUIRES "llvm-toolset-clang >= 10.0.1, llvm-toolset-lld >= 10.0.1, libcxx-devel >= 10.0.1, libcxxabi-devel >= 10.0.1")


# Note: set this variable to use a custom RPM spec file.
# ATM the file does not work. https://gaiaplatform.atlassian.net/browse/GAIAPLAT-592
# set(CPACK_RPM_USER_BINARY_SPECFILE ${PROJECT_SOURCE_DIR}/rpm/gaia.spec)

# Set CPackDebHelper variables:
#
# The directory that contains the files listed in CPACK_DEBHELPER_INPUT.
set(CPACK_DEBHELPER_INPUT_DIR "${PROJECT_SOURCE_DIR}/debian")
# List of input files that require processing by the debhelpers. All of these
# must have the '.in' extension because they are first processed by
# configure_file().
set(CPACK_DEBHELPER_INPUT
  control.in
  changelog.in
  service.in
  links.in
  dirs.in
)
# List of debhelpers to run.
# If left unset the default list of debhelpers is executed.
set(CPACK_DEBHELPER_RUN
  dh_install dh_installdirs dh_installchangelogs
  dh_installdocs dh_installsystemd
  dh_usrlocal dh_link dh_dwz dh_compress dh_fixperms)
# Let the debhelpers generate the debian/control file (instead of CPack).
set(CPACK_DEBHELPER_GENCONTROL ON)
# Run the debhelpers in verbose mode.
set(CPACK_DEBHELPER_VERBOSE ON)

# Build rpm
set(CPACK_GENERATOR "RPM")

if((BUILD_GAIA_RELEASE) AND (NOT BUILD_GAIA_LLVM_TESTS))
  # Load the CPackDebHelper module, this will:
  #   - Add "DEB" to the CPACK_GENERATOR variable.
  #   - Parse the files in CPACK_DEBHELPER_INPUT with configure_file().
  #   - Set the CPACK_DEBIAN_INSTALL_SCRIPT variable to
  #     ${CMAKE_CURRENT_LIST_DIR}/CPackDebHelperInstall.cmake.
  #   - Set CPACK_PROJECT_CONFIG_FILE variable to
  #     ${CMAKE_CURRENT_LIST_DIR}/CPackDebHelperConfig.cmake.
  #
  # When the 'make package' or 'cpack' command is executed CPackDebHelper will:
  #   - Run the debhelpers specified in CPACK_DEBHELPER_RUN.
  #   - Add the generated files in the 'debian' directory to
  #     CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA.
  #   - Copy all other files generated by debhelper to the packaging area.
  include(CPackDebHelper)
  message(CHECK_START "Looking for debhelper")
  if(CPACK_DEBHELPER_FATALITY)
    message(CHECK_FAIL "not found")
  else()
    message(CHECK_PASS "found")
  endif()
endif()
# Load the CPack module.
include(CPack)

# Place the README.md file.
install(FILES ${GAIA_REPO}/production/sdk/README.md DESTINATION .)

# Place flatbuffer headers.
install(FILES ${FLATBUFFERS_INC}/flatbuffers/flatbuffers.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/flatbuffers)
install(FILES ${FLATBUFFERS_INC}/flatbuffers/base.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/flatbuffers)
install(FILES ${FLATBUFFERS_INC}/flatbuffers/stl_emulation.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/flatbuffers)

# Place gaia_spdlog headers.
install(DIRECTORY ${GAIA_SPDLOG_INC}/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Place gaia headers.
install(DIRECTORY ${GAIA_INC}/gaia DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Place gaia configs.
install(FILES ${GAIA_REPO}/production/gaia.conf DESTINATION ${CMAKE_INSTALL_SYSCONFDIR})
install(FILES ${GAIA_REPO}/production/gaia_log.conf DESTINATION ${CMAKE_INSTALL_SYSCONFDIR})

# Place cmake functions
install(FILES ${GAIA_REPO}/production/cmake/gaia.cmake DESTINATION cmake/)

# Place hello example.
install(FILES ${GAIA_REPO}/production/examples/hello/README.md DESTINATION examples/hello)
install(FILES ${GAIA_REPO}/production/examples/hello/hello.ddl DESTINATION examples/hello)
install(FILES ${GAIA_REPO}/production/examples/hello/hello.ruleset DESTINATION examples/hello)
install(FILES ${GAIA_REPO}/production/examples/hello/hello.cpp DESTINATION examples/hello)
install(FILES ${GAIA_REPO}/production/examples/hello/setup.sql DESTINATION examples/hello)
install(PROGRAMS ${GAIA_REPO}/production/examples/hello/setup.sh DESTINATION examples/hello)
install(PROGRAMS ${GAIA_REPO}/production/examples/hello/build.sh DESTINATION examples/hello)
install(PROGRAMS ${GAIA_REPO}/production/examples/hello/run.sh DESTINATION examples/hello)
install(FILES ${GAIA_REPO}/production/examples/hello/CMakeLists.txt DESTINATION examples/hello)

# Place incubator example.
install(FILES ${GAIA_REPO}/production/examples/incubator/incubator.ddl DESTINATION examples/incubator)
install(FILES ${GAIA_REPO}/production/examples/incubator/incubator.ruleset DESTINATION examples/incubator)
install(FILES ${GAIA_REPO}/production/examples/incubator/incubator.cpp DESTINATION examples/incubator)
install(FILES ${GAIA_REPO}/production/examples/incubator/README.md DESTINATION examples/incubator)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  install(FILES ${GAIA_REPO}/production/examples/incubator/CMakeLists_debug.txt DESTINATION examples/incubator RENAME CMakeLists.txt)
else()
  install(FILES ${GAIA_REPO}/production/examples/incubator/CMakeLists.txt DESTINATION examples/incubator)
endif()

# Place direct_access example
install(FILES ${GAIA_REPO}/production/examples/direct_access/hospital.ddl DESTINATION examples/direct_access)
install(FILES ${GAIA_REPO}/production/examples/direct_access/hospital.cpp DESTINATION examples/direct_access)
install(FILES ${GAIA_REPO}/production/examples/direct_access/CMakeLists.txt DESTINATION examples/direct_access)
install(FILES ${GAIA_REPO}/production/examples/direct_access/README.md DESTINATION examples/direct_access)

# Place license files.
install(FILES ${GAIA_REPO}/production/licenses/LICENSE.third-party.txt DESTINATION .)
