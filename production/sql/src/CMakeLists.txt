# ##############################################################################
# Copyright (c) Gaia Platform LLC. All rights reserved.
# ##############################################################################

add_library(gaia_fdw SHARED
  gaia_fdw.cpp
  gaia_fdw_adapter.cpp)

configure_gaia_target(gaia_fdw)

# exec_program() mangles this variable with added quotes, so un-quote it.
separate_arguments(PGSQL_CPPFLAGS UNIX_COMMAND "${PGSQL_CPPFLAGS}")
target_compile_options(gaia_fdw PRIVATE ${PGSQL_CPPFLAGS})

# exec_program() mangles this variable with added quotes, so un-quote it.
separate_arguments(PGSQL_LDFLAGS UNIX_COMMAND "${PGSQL_LDFLAGS}")
target_link_options(gaia_fdw PRIVATE ${PGSQL_LDFLAGS})

if("$CACHE{SANITIZER}" STREQUAL "ASAN")
  target_link_options(gaia_fdw PRIVATE "-shared-libasan")
endif()

set_target_properties(
  gaia_fdw PROPERTIES
    OUTPUT_NAME "gaia_fdw-${PG_GAIA_FDW_VERSION}"
    PREFIX "")

target_include_directories(gaia_fdw PRIVATE "${PROJECT_SOURCE_DIR}/inc")
target_include_directories(gaia_fdw PRIVATE "${GAIA_INC}")
target_include_directories(gaia_fdw PRIVATE "${FLATBUFFERS_INC}")
target_include_directories(gaia_fdw SYSTEM PRIVATE "${PGSQL_INCLUDEDIR_SERVER}")

target_link_libraries(gaia_fdw PRIVATE gaia_db_client)
target_link_libraries(gaia_fdw PRIVATE gaia_catalog)
target_link_libraries(gaia_fdw PRIVATE gaia_payload_types)

# Extension configuration.
set(GAIA_FDW_INSTALL_EXTENSIONS
  "${PROJECT_BINARY_DIR}/pgsql/gaia_fdw--${PG_GAIA_FDW_VERSION}.sql"
  "${PROJECT_BINARY_DIR}/pgsql/gaia_fdw.control")
configure_file(
  gaia_fdw.sql
  "${PROJECT_BINARY_DIR}/pgsql/gaia_fdw--${PG_GAIA_FDW_VERSION}.sql")
configure_file(gaia_fdw.control "${PROJECT_BINARY_DIR}/pgsql/gaia_fdw.control")

# Installation steps for configuration and binaries.
install(TARGETS gaia_fdw DESTINATION ${PGSQL_PKGLIBDIR})
install(FILES ${GAIA_FDW_INSTALL_EXTENSIONS}
  DESTINATION "${PGSQL_SHAREDIR}/extension")
