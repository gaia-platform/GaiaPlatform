#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# compile config

set (COW_SE_SOURCES
  cow_se_fdw.cpp
)

set (CMAKE_C_FLAGS "${PGSQL_CPPFLAGS}")
set (CMAKE_SHARED_LINKER_FLAGS "${PGSQL_LDFLAGS}")

add_executable (data_loader data_loader.cpp)
# for COW-SE
target_include_directories (data_loader PRIVATE "../../../../cow_se/inc")
# for flatbuffers generated code
target_include_directories (data_loader SYSTEM PRIVATE "../../../flatbuffers")
# for POSIX shared memory API
target_link_libraries (data_loader rt)

add_library (cow_se_fdw SHARED ${COW_SE_SOURCES})

set_target_properties (cow_se_fdw PROPERTIES
  OUTPUT_NAME "cow_se_fdw-${PGCOW_SE_VERSION_MAJOR}"
  PREFIX ""
)

# for COW-SE
target_include_directories (cow_se_fdw PRIVATE "../../../../cow_se/inc")
# for flatbuffers generated code
target_include_directories (cow_se_fdw SYSTEM PRIVATE "../../../flatbuffers")
# for postgres
target_include_directories (cow_se_fdw SYSTEM PRIVATE "${PGSQL_INCLUDEDIR_SERVER}")

# extension config

set ( COW_SE_INSTALL_EXTENSIONS
  "${PROJECT_BINARY_DIR}/pgsql/cow_se_fdw--${PGCOW_SE_VERSION}.sql"
  "${PROJECT_BINARY_DIR}/pgsql/cow_se_fdw.control"
)

configure_file (
  cow_se_fdw.sql
  "${PROJECT_BINARY_DIR}/pgsql/cow_se_fdw--${PGCOW_SE_VERSION}.sql"
)

configure_file (
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

install (
  TARGETS data_loader
  DESTINATION "/usr/local/bin"
)
