#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# compile config

set (COW_SE_SOURCES
  cow_se_fdw.c
)

set (COW_SE_HEADERS
  cow_se_fdw.h
)

set(CMAKE_C_FLAGS "${PGSQL_CPPFLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${PGSQL_LDFLAGS}")

include_directories ("${PGSQL_INCLUDEDIR_SERVER}")

add_library (cow_se_fdw SHARED ${COW_SE_SOURCES} ${COW_SE_HEADERS})

set_target_properties (cow_se_fdw PROPERTIES
  OUTPUT_NAME "cow_se_fdw-${PGCOW_SE_VERSION_MAJOR}"
  PREFIX ""
)

target_link_libraries(cow_se_fdw m)

# extension config

set ( COW_SE_INSTALL_EXTENSIONS
  "${PROJECT_BINARY_DIR}/pgsql/cow_se_fdw--${PGCOW_SE_VERSION}.sql"
  "${PROJECT_BINARY_DIR}/pgsql/cow_se_fdw.control"
)

configure_file(
  cow_se_fdw.sql
  "${PROJECT_BINARY_DIR}/pgsql/cow_se_fdw--${PGCOW_SE_VERSION}.sql"
)

configure_file(
  cow_se_fdw.control
  "${PROJECT_BINARY_DIR}/pgsql/cow_se_fdw.control"
)

# install config and binaries

install (
  TARGETS cow_se_fdw
  DESTINATION ${PGSQL_PKGLIBDIR}
)

install (
  FILES ${COW_SE_INSTALL_EXTENSIONS}
  DESTINATION "${PGSQL_SHAREDIR}/extension"
)
