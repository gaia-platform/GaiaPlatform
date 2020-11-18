# ##############################################################################
# Copyright (c) Gaia Platform LLC. All rights reserved.
# ##############################################################################

add_library(gaia_fdw SHARED
  gaia_fdw.cpp
  gaia_fdw_adapter.cpp)

# exec_program() mangles this variable with added quotes, so un-quote it.
separate_arguments(PGSQL_CPPFLAGS UNIX_COMMAND "${PGSQL_CPPFLAGS}")
target_compile_options(gaia_fdw PRIVATE ${PGSQL_CPPFLAGS})

# We need to un-quote this variable for some reason as well.
separate_arguments(GAIA_COMPILE_FLAGS UNIX_COMMAND "${GAIA_COMPILE_FLAGS}")
target_compile_options(gaia_fdw PRIVATE ${GAIA_COMPILE_FLAGS})

# exec_program() mangles this variable with added quotes, so un-quote it.
separate_arguments(PGSQL_LDFLAGS UNIX_COMMAND "${PGSQL_LDFLAGS}")
target_link_options(gaia_fdw PRIVATE ${PGSQL_LDFLAGS})

# We need to un-quote this variable for some reason as well.
separate_arguments(GAIA_LINK_FLAGS UNIX_COMMAND "${GAIA_LINK_FLAGS}")
target_link_options(gaia_fdw PRIVATE ${GAIA_LINK_FLAGS})

set_target_properties(
  gaia_fdw PROPERTIES
    OUTPUT_NAME "gaia_fdw-${PG_GAIA_FDW_VERSION_MAJOR}"
    PREFIX "")

target_include_directories(gaia_fdw PRIVATE "${PROJECT_SOURCE_DIR}/inc")
target_include_directories(gaia_fdw PRIVATE "${GAIA_INC}")
target_include_directories(gaia_fdw PRIVATE "${GAIA_INC}")
target_include_directories(gaia_fdw PRIVATE "${GAIA_INC}")
target_include_directories(gaia_fdw PRIVATE "${GAIA_INC}")
target_include_directories(gaia_fdw PRIVATE "${GAIA_INC}")
target_include_directories(gaia_fdw PRIVATE "${GAIA_INC}/gaia_internal/db")
target_include_directories(gaia_fdw PRIVATE "${GAIA_INC}/db/payload_types")
target_include_directories(gaia_fdw SYSTEM PRIVATE "${PGSQL_INCLUDEDIR_SERVER}")

target_link_libraries(gaia_fdw PRIVATE gaia_se_client)
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
