#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.17)

project(sdk)

set(TEMP_PACKAGE_FOLDER "${CMAKE_CURRENT_BINARY_DIR}/tmp")

include(gaia)

file(WRITE ${CMAKE_BINARY_DIR}/sdk_dummy.c "")

add_library(gaia_system SHARED ${CMAKE_BINARY_DIR}/sdk_dummy.c)
set(SDK_LINK_LIBRARIES
  -Wl,--whole-archive
  gaia_direct
  gaia_rules
  gaia
  gaia_parser
  gaia_catalog
  gaia_common
  gaia_payload_types
  gaia_se_client
  rocks_wrapper
  flatbuffers
  -Wl,--no-whole-archive
)

target_link_libraries(gaia_system PUBLIC
  ${SDK_LINK_LIBRARIES}
)

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
  COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/db/storage_engine/se_client.cpython-38-x86_64-linux-gnu.so ${TEMP_PACKAGE_FOLDER}/usr/local/lib/se_client.cpython-38-x86_64-linux-gnu.so
  COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/db/index/libgaia_index.so ${TEMP_PACKAGE_FOLDER}/usr/local/lib/libgaia_index.so
  COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/db/memory_manager/libgaia_memorymanager.so ${TEMP_PACKAGE_FOLDER}/usr/local/lib/libgaia_memorymanager.so
  COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/db/triggers/libgaia_triggers.so ${TEMP_PACKAGE_FOLDER}/usr/local/lib/libgaia_triggers.so
  COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/sql/src/gaia_fdw-0.0.so ${TEMP_PACKAGE_FOLDER}/usr/local/lib/gaia_fdw-0.0.so
  COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/rocksdb/librocksdb.so ${TEMP_PACKAGE_FOLDER}/usr/local/lib/librocksdb.so.6

  COMMAND cmake -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/libgaia_system.so ${TEMP_PACKAGE_FOLDER}/usr/local/lib/libgaia_system.so

  COMMAND cmake -E copy_if_different /usr/lib/x86_64-linux-gnu/libexplain.so.51.5.0 ${TEMP_PACKAGE_FOLDER}/usr/lib/x86_64-linux-gnu/libexplain.so.51
  COMMAND cmake -E copy_if_different /usr/local/lib/libfmt.so.7.0.3 ${TEMP_PACKAGE_FOLDER}/usr/local/lib/libfmt.so.7
  COMMAND cmake -E copy_if_different /usr/local/lib/libspdlog.so.1.8.0 ${TEMP_PACKAGE_FOLDER}/usr/local/lib/libspdlog.so.1
  COMMAND cmake -E copy_if_different /usr/lib/x86_64-linux-gnu/libcap.so.2.32 ${TEMP_PACKAGE_FOLDER}/usr/lib/x86_64-linux-gnu/libcap.so.2
  COMMAND cmake -E copy_if_different /usr/lib/x86_64-linux-gnu/libcap-ng.so.0.0.0 ${TEMP_PACKAGE_FOLDER}/usr/lib/x86_64-linux-gnu/libcap-ng.so.0

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
  COMMENT "Compiled sdk packages."
)

add_custom_target(generate_gaia_deb_package ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/gaia.deb
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/gaia.rpm
)
