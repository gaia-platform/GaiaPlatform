#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.12)

project(sdk)

set(TEMP_PACKAGE_FOLDER "${CMAKE_CURRENT_BINARY_DIR}/tmp")

include(gaia)

add_custom_command(
  OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/gaia.deb
  OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/gaia.rpm

  COMMAND cmake -E copy_if_different ${PROJECT_SOURCE_DIR}/control ${TEMP_PACKAGE_FOLDER}/DEBIAN/control
  COMMAND cmake -E copy_if_different ${PROJECT_SOURCE_DIR}/changelog ${TEMP_PACKAGE_FOLDER}/DEBIAN/changelog
  COMMAND cmake -E copy_if_different ${PROJECT_SOURCE_DIR}/copyright ${TEMP_PACKAGE_FOLDER}/DEBIAN/copyright

  COMMAND cmake -E copy_directory ${GAIA_INC}/public ${TEMP_PACKAGE_FOLDER}/usr/local/include/gaia

  COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/db/storage_engine/gaia_se_server ${TEMP_PACKAGE_FOLDER}/usr/local/bin/gaia_se_server
  COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/catalog/gaiac/gaiac ${TEMP_PACKAGE_FOLDER}/usr/local/bin/gaiac
  COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/tools/gaia_translate/gaiat ${TEMP_PACKAGE_FOLDER}/usr/local/bin/gaiat

  COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/direct_access/libgaia_direct.a  ${TEMP_PACKAGE_FOLDER}/usr/local/lib/libgaia_direct.a
  COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/rules/event_manager/libgaia_rules.a  ${TEMP_PACKAGE_FOLDER}/usr/local/lib/libgaia_rules.a
  COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/system/libgaia.a  ${TEMP_PACKAGE_FOLDER}/usr/local/lib/libgaia.a
  COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/catalog/parser/libgaia_parser.a  ${TEMP_PACKAGE_FOLDER}/usr/local/lib/libgaia_parser.a
  COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/catalog/catalog_manager/libgaia_catalog.a  ${TEMP_PACKAGE_FOLDER}/usr/local/lib/libgaia_catalog.a
  COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/common/libgaia_common.a  ${TEMP_PACKAGE_FOLDER}/usr/local/lib/libgaia_common.a
  COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/db/types/libgaia_payload_types.a  ${TEMP_PACKAGE_FOLDER}/usr/local/lib/libgaia_payload_types.a
  COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/db/storage_engine/libgaia_se_client.a  ${TEMP_PACKAGE_FOLDER}/usr/local/lib/libgaia_se_client.a
  COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/db/storage_engine/librocks_wrapper.a  ${TEMP_PACKAGE_FOLDER}/usr/local/lib/librocks_wrapper.a
  COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/flatbuffers/libflatbuffers.a  ${TEMP_PACKAGE_FOLDER}/usr/local/lib/libflatbuffers.a

  COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/llvm/lib/libclang.so ${TEMP_PACKAGE_FOLDER}/usr/local/lib/libclang.so
  COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/db/storage_engine/libgaia_storage.so ${TEMP_PACKAGE_FOLDER}/usr/local/lib/libgaia_storage.so
  COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/db/storage_engine/se_client.cpython-38-x86_64-linux-gnu.so ${TEMP_PACKAGE_FOLDER}/usr/local/lib/se_client.cpython-38-x86_64-linux-gnu.so
  COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/db/index/libgaia_index.so ${TEMP_PACKAGE_FOLDER}/usr/local/lib/libgaia_index.so
  COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/db/memory_manager/libgaia_memorymanager.so ${TEMP_PACKAGE_FOLDER}/usr/local/lib/libgaia_memorymanager.so
  COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/db/triggers/libgaia_triggers.so ${TEMP_PACKAGE_FOLDER}/usr/local/lib/libgaia_triggers.so
  COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/sql/src/gaia_fdw-0.0.so ${TEMP_PACKAGE_FOLDER}/usr/local/lib/gaia_fdw-0.0.so
  COMMAND cmake -E copy_if_different ${GAIA_PROD_BUILD}/rocksdb/librocksdb.so ${TEMP_PACKAGE_FOLDER}/usr/local/lib/librocksdb.so

  COMMAND dpkg-deb --build ${TEMP_PACKAGE_FOLDER} ${CMAKE_CURRENT_BINARY_DIR}/gaia.deb

  COMMAND cmake -E rm -r ${TEMP_PACKAGE_FOLDER}/DEBIAN

  COMMAND rpmbuild --bb ${PROJECT_SOURCE_DIR}/gaia.spec --define "_rpmdir ${CMAKE_CURRENT_BINARY_DIR}" --define "buildroot ${TEMP_PACKAGE_FOLDER}" --define "_build_name_fmt gaia.rpm"

  COMMAND cmake -E rm -r ${TEMP_PACKAGE_FOLDER}
  DEPENDS gaiac
  DEPENDS gaiat
  DEPENDS gaia_se_server
  DEPENDS gaia_direct
  DEPENDS gaia_rules
  DEPENDS gaia
  DEPENDS gaia_parser
  DEPENDS gaia_catalog
  DEPENDS gaia_common
  DEPENDS gaia_payload_types
  DEPENDS gaia_se_client
  DEPENDS rocks_wrapper
  DEPENDS flatbuffers
  DEPENDS ${PROJECT_SOURCE_DIR}/control
  DEPENDS ${PROJECT_SOURCE_DIR}/changelog
  DEPENDS ${PROJECT_SOURCE_DIR}/copyright
  DEPENDS ${PROJECT_SOURCE_DIR}/gaia.spec
  COMMENT "Compiled sdk packages."
)

add_custom_target(generate_gaia_deb_package ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/gaia.deb
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/gaia.rpm
)
