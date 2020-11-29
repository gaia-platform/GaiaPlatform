#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

project(sdk)

set(TEMP_PACKAGE_FOLDER "${CMAKE_CURRENT_BINARY_DIR}/tmp")
set(PGSQL_PACKAGE_FOLDER "${TEMP_PACKAGE_FOLDER}/opt/gaia/lib/pgsql_ext")
set(GAIA_PACKAGE_VERSION "0.1.0")
set(GAIA_DEB_PACKAGE_NAME "gaia_${GAIA_PACKAGE_VERSION}_amd64.deb")
set(GAIA_LIBRARY_NAME "libgaia.so.${GAIA_PACKAGE_VERSION}")

include(gaia)

file(WRITE ${CMAKE_BINARY_DIR}/sdk_dummy.c "")

# TODO: Replace dummy source with object libraries.
add_library(gaia SHARED ${CMAKE_BINARY_DIR}/sdk_dummy.c)
set(SDK_LINK_LIBRARIES
  -Wl,--whole-archive
  gaia_system
  -Wl,--no-whole-archive
)

target_link_libraries(gaia PRIVATE
  ${SDK_LINK_LIBRARIES}
)

install(TARGETS gaia DESTINATION ${CMAKE_INSTALL_LIBDIR})

#
# Ensure this test only includes public headers and links to the gaia shared library.
#
set(SDK_TEST_INCLUDES
  ${GAIA_INC}
  ${FLATBUFFERS_INC}
  ${GAIA_GENERATED_SCHEMAS}
)

configure_file("${GAIA_CONFIG}" "${PROJECT_BINARY_DIR}/gaia.conf")
configure_file("${GAIA_LOG_CONFIG}" "${PROJECT_BINARY_DIR}/gaia_log.conf")
add_gtest(test_sdk "tests/test_sdk.cpp" "${SDK_TEST_INCLUDES}" "rt;gaia" "generate_addr_book_headers")


# Set CPack variables required for building a DEB package.
#
# See:
# https://cmake.org/cmake/help/v3.17/module/CPack.html#variables-common-to-all-cpack-generators
# https://cmake.org/cmake/help/v3.17/cpack_gen/deb.html#variables-specific-to-cpack-debian-deb-generator
#
# Required. Debian packages need a maintainer.
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "info@gaiaplatform.io")
# Optional. Set to the project homepage.
set(CPACK_PACKAGE_HOMEPAGE_URL "https://www.gaiaplatform.io/")
# Optional. Set to PROJECT_NAME by CPackDebHelper if left unset.
set(CPACK_PACKAGE_NAME "gaia")
# Optional. Default to "0.1.1".
set(CPACK_PACKAGE_VERSION_MAJOR "0.1.0")
# Optional. Default to "1".
set(CPACK_PACKAGE_VERSION_MINOR "0")
# Optional. Set to CMAKE_BINARY_DIR by CPackDebHelper if left unset.
set(CPACK_PACKAGE_DIRECTORY ${CMAKE_BINARY_DIR})
# Optional. Default to "/usr".
set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/gaia")
# Optional. The DEB package likes to know its target system architecture.
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE amd64)
# Optional. This is used in the debian/control file.
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Gaia Platform SDK")
# Optional.
set(CPACK_DEBIAN_FILE_NAME ${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb)
# Optional. CPack produces a warning if CPACK_DEBIAN_PACKAGE_DEPENDS is also
# empty (ie. no dependencies are given at all).
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)

# Set CPackDebHelper variables:
#
# Required. The directory that contains the files listed in CPACK_DEBHELPER_INPUT
set(CPACK_DEBHELPER_INPUT_DIR "${PROJECT_SOURCE_DIR}/debian")
# Required. List of input files that require processing by the debhelpers. All
# of these must have the '.in' extention because they are first processed by
# configure_file().
set(CPACK_DEBHELPER_INPUT
  control.in
  changelog.in
  service.in
  links.in
)
# Optional. List of debhelpers to run.
# If left unset the default list of debhelpers is executed.
set(CPACK_DEBHELPER_RUN
  dh_install dh_installdirs dh_installchangelogs
  dh_installdocs dh_installsystemd
  dh_usrlocal dh_link dh_dwz dh_compress dh_fixperms)
# Optional. Scan for shared libraries and generates a shlibs file.
# If left unset then CPackDebHelper set this to OFF.
set(CPACK_DEBHELPER_MAKESHLIBS OFF)
# Optional. Generate shared library substvar dependencies.
# If left unset then CPackDebHelper sets this to CPACK_DEBIAN_PACKAGE_SHLIBDEPS
# or to ON if CPACK_DEBIAN_PACKAGE_SHLIBDEPS is not defined.
set(CPACK_DEBHELPER_SHLIBDEPS OFF)
# Optional. Let the debhelpers generate the debian/control file (instead of CPack)
# If left unset then the CPackDebHelper set this to ON because the CPack DEB
# generator does not create a valid control file.
set(CPACK_DEBHELPER_GENCONTROL ON)
# Optional. Run the debhelpers in verbose mode.
set(CPACK_DEBHELPER_VERBOSE ON)

# Load the CPackDebHelper module.
include(CPackDebHelper)
# If CPACK_DEBHELPER_FATALITY is true at this point a DEB package may still be
# built but will not contain any debhelper generated files.
message(CHECK_START "Looking for debhelper")
if(CPACK_DEBHELPER_FATALITY)
  message(CHECK_FAIL "not found")
else()
  message(CHECK_PASS "found")
endif()
# Load the CPack module.
include(CPack)

# Place flatbuffer headers.
install(FILES ${FLATBUFFERS_INC}/flatbuffers/flatbuffers.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/flatbuffers)
install(FILES ${FLATBUFFERS_INC}/flatbuffers/base.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/flatbuffers)
install(FILES ${FLATBUFFERS_INC}/flatbuffers/stl_emulation.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/flatbuffers)

# Place gaia headers.
install(DIRECTORY ${GAIA_INC}/gaia DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Place gaia configs
install(FILES ${GAIA_REPO}/production/gaia.conf DESTINATION ${CMAKE_INSTALL_SYSCONFDIR})
install(FILES ${GAIA_REPO}/production/gaia_log.conf DESTINATION ${CMAKE_INSTALL_SYSCONFDIR})

# Place incubator examples
install(FILES ${GAIA_REPO}/demos/incubator/gaia.conf DESTINATION examples/incubator_demo)
install(FILES ${GAIA_REPO}/demos/incubator/gaia_log.conf DESTINATION examples/incubator_demo)
install(FILES ${GAIA_REPO}/demos/incubator/incubator.ddl DESTINATION examples/incubator_demo)
install(FILES ${GAIA_REPO}/demos/incubator/incubator.ruleset DESTINATION examples/incubator_demo)
install(FILES ${GAIA_REPO}/demos/incubator/main.cpp DESTINATION examples/incubator_demo)
install(FILES ${GAIA_REPO}/demos/incubator/README.md DESTINATION examples/incubator_demo)
install(FILES ${PROJECT_SOURCE_DIR}/CMakeLists-incubator.txt DESTINATION examples/incubator_demo RENAME CMakeLists.txt)
