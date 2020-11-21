#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

project(sdk)

set(TEMP_PACKAGE_FOLDER "${CMAKE_CURRENT_BINARY_DIR}/tmp")

include(gaia)

file(WRITE ${CMAKE_BINARY_DIR}/sdk_dummy.c "")

add_library(gaia SHARED ${CMAKE_BINARY_DIR}/sdk_dummy.c)
set(SDK_LINK_LIBRARIES
  -Wl,--whole-archive
  gaia_system
  -Wl,--no-whole-archive
)

target_link_libraries(gaia PRIVATE
  ${SDK_LINK_LIBRARIES}
)

if(BUILD_GAIA_RELEASE)
  add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/gaia.deb
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/gaia.rpm

    COMMAND cmake -E copy_if_different ${PROJECT_SOURCE_DIR}/control ${TEMP_PACKAGE_FOLDER}/DEBIAN/control
    COMMAND cmake -E copy_if_different ${PROJECT_SOURCE_DIR}/changelog ${TEMP_PACKAGE_FOLDER}/DEBIAN/changelog
    COMMAND cmake -E copy_if_different ${PROJECT_SOURCE_DIR}/copyright ${TEMP_PACKAGE_FOLDER}/DEBIAN/copyright
    COMMAND cmake -E copy_if_different ${PROJECT_SOURCE_DIR}/preinst ${TEMP_PACKAGE_FOLDER}/DEBIAN/preinst
    COMMAND cmake -E copy_if_different ${PROJECT_SOURCE_DIR}/postinst ${TEMP_PACKAGE_FOLDER}/DEBIAN/postinst

    COMMAND cmake -E copy_directory ${GAIA_INC}/public ${TEMP_PACKAGE_FOLDER}/usr/local/include/gaia

    COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/db/storage_engine/gaia_se_server ${TEMP_PACKAGE_FOLDER}/usr/local/bin/gaia_se_server
    COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/catalog/gaiac/gaiac ${TEMP_PACKAGE_FOLDER}/usr/local/bin/gaiac
    COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/tools/gaia_translate/gaiat ${TEMP_PACKAGE_FOLDER}/usr/local/bin/gaiat

    COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/llvm/lib/libclang.so ${TEMP_PACKAGE_FOLDER}/usr/local/lib/libclang.so
    COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/db/storage_engine/libgaia_storage.so ${TEMP_PACKAGE_FOLDER}/usr/local/lib/libgaia_storage.so
    COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/db/index/libgaia_index.so ${TEMP_PACKAGE_FOLDER}/usr/local/lib/libgaia_index.so
    COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/db/memory_manager/libgaia_memorymanager.so ${TEMP_PACKAGE_FOLDER}/usr/local/lib/libgaia_memorymanager.so
    COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/db/triggers/libgaia_triggers.so ${TEMP_PACKAGE_FOLDER}/usr/local/lib/libgaia_triggers.so
    COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/sql/src/gaia_fdw-0.0.so ${TEMP_PACKAGE_FOLDER}/usr/local/lib/gaia_fdw-0.0.so
    COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/rocksdb/librocksdb.so ${TEMP_PACKAGE_FOLDER}/usr/local/lib/librocksdb.so.6

    COMMAND cmake -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/libgaia.so ${TEMP_PACKAGE_FOLDER}/usr/local/lib/libgaia.so

    COMMAND dpkg-deb --build ${TEMP_PACKAGE_FOLDER} ${CMAKE_CURRENT_BINARY_DIR}/gaia.deb

    COMMAND cmake -E rm -r ${TEMP_PACKAGE_FOLDER}/DEBIAN

    COMMAND rpmbuild --bb ${PROJECT_SOURCE_DIR}/gaia.spec --define "_rpmdir ${CMAKE_CURRENT_BINARY_DIR}" --define "buildroot ${TEMP_PACKAGE_FOLDER}" --define "_build_name_fmt gaia.rpm"

    COMMAND cmake -E rm -r ${TEMP_PACKAGE_FOLDER}
    DEPENDS gaiac
    DEPENDS gaiat
    DEPENDS gaia_se_server
    DEPENDS gaia_system
    DEPENDS libclang
    DEPENDS ${PROJECT_SOURCE_DIR}/control
    DEPENDS ${PROJECT_SOURCE_DIR}/preinst
    DEPENDS ${PROJECT_SOURCE_DIR}/postinst
    DEPENDS ${PROJECT_SOURCE_DIR}/changelog
    DEPENDS ${PROJECT_SOURCE_DIR}/copyright
    DEPENDS ${PROJECT_SOURCE_DIR}/gaia.spec
    COMMENT "Compiled SDK packages."
  )

  add_custom_target(generate_gaia_deb_package ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/gaia.deb
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/gaia.rpm
  )
endif()

#
# Ensure this test only includes public headers and links to the gaia shared library.
#
set(SDK_TEST_INCLUDES
  ${GAIA_INC}/public/rules
  ${GAIA_INC}/public/direct_access
  ${GAIA_INC}/public/db
  ${GAIA_INC}/public/common
  ${GAIA_INC}/public/system
  ${FLATBUFFERS_INC}
  ${GAIA_GENERATED_SCHEMAS}
)

configure_file("${GAIA_CONFIG}" "${PROJECT_BINARY_DIR}/gaia.conf")
configure_file("${GAIA_LOG_CONFIG}" "${PROJECT_BINARY_DIR}/gaia_log.conf")
add_gtest(test_sdk "tests/test_sdk.cpp" "${SDK_TEST_INCLUDES}" "rt;gaia" "generate_addr_book_headers")
