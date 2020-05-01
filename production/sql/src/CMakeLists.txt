#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Generate flatbuffers headers.

find_program (FLATCC flatcc)
set (GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set (FBS_SOURCES
     "${GAIA_REPO}/demos/airport_q1/inc/airport_q1.fbs"
     "${GAIA_REPO}/production/rules/event_manager/flatbuffers/event_log.fbs"
)
add_custom_target(gen_fbs ALL)
foreach (fbs ${FBS_SOURCES})
  add_custom_command (
      TARGET gen_fbs
      COMMAND cmake -E make_directory "${GEN_DIR}"
      COMMAND "${FLATCC}" -a -o "${GEN_DIR}" "${fbs}"
      DEPENDS "${fbs}"
      VERBATIM
  )
endforeach (fbs)

# Compile config.

add_library (gaia_fdw SHARED gaia_fdw.cpp)
message("PGSQL_CPPFLAGS ${PGSQL_CPPFLAGS}")
message("PGSQL_LDFLAGS ${PGSQL_LDFLAGS}")
# exec_program() mangles this variable with added quotes, so un-quote it
separate_arguments(PGSQL_CPPFLAGS UNIX_COMMAND "${PGSQL_CPPFLAGS}")
target_compile_options (gaia_fdw PRIVATE ${PGSQL_CPPFLAGS})
# we need to un-quote this variable for some reason as well
separate_arguments(GAIA_COMPILE_FLAGS UNIX_COMMAND "${GAIA_COMPILE_FLAGS}")
target_compile_options (gaia_fdw PRIVATE ${GAIA_COMPILE_FLAGS})
# exec_program() mangles this variable with added quotes, so un-quote it
separate_arguments(PGSQL_LDFLAGS UNIX_COMMAND "${PGSQL_LDFLAGS}")
target_link_options (gaia_fdw PRIVATE ${PGSQL_LDFLAGS})
# we need to un-quote this variable for some reason as well
separate_arguments(GAIA_LINK_FLAGS UNIX_COMMAND "${GAIA_LINK_FLAGS}")
target_link_options (gaia_fdw PRIVATE ${GAIA_LINK_FLAGS})
set_target_properties (gaia_fdw PROPERTIES
  OUTPUT_NAME "gaia_fdw-${PG_GAIA_FDW_VERSION_MAJOR}"
  PREFIX ""
)
add_dependencies(gaia_fdw gen_fbs)
# our private headers
target_include_directories (gaia_fdw PRIVATE "${PROJECT_SOURCE_DIR}/inc")
# for flatbuffers generated code
target_include_directories (gaia_fdw SYSTEM PRIVATE "${GEN_DIR}")
# for postgres
target_include_directories (gaia_fdw SYSTEM PRIVATE "${PGSQL_INCLUDEDIR_SERVER}")
# for flatbuffers C builder support
target_link_libraries (gaia_fdw PRIVATE flatccrt)
# for COW-SE
target_link_libraries (gaia_fdw PRIVATE gaia_semock)
# we want to link to a static lib for the SE, but there are no decl-only headers yet for it
target_include_directories (gaia_fdw PRIVATE "${GAIA_REPO}/production/db/storage_engine/mock")

# Extension config.

set ( GAIA_FDW_INSTALL_EXTENSIONS
  "${PROJECT_BINARY_DIR}/pgsql/gaia_fdw--${PG_GAIA_FDW_VERSION}.sql"
  "${PROJECT_BINARY_DIR}/pgsql/gaia_fdw.control"
)

configure_file (
  gaia_fdw.sql
  "${PROJECT_BINARY_DIR}/pgsql/gaia_fdw--${PG_GAIA_FDW_VERSION}.sql"
)

configure_file (
  gaia_fdw.control
  "${PROJECT_BINARY_DIR}/pgsql/gaia_fdw.control"
)

# Install config and binaries.

install (
  TARGETS gaia_fdw
  DESTINATION ${PGSQL_PKGLIBDIR}
)

install (
  FILES ${GAIA_FDW_INSTALL_EXTENSIONS}
  DESTINATION "${PGSQL_SHAREDIR}/extension"
)
