# ##############################################################################
# Copyright (c) Gaia Platform LLC. All rights reserved.
# ##############################################################################

# Generate flatbuffers headers.

find_program(FLATCC flatcc)

set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

set(FBS_SOURCES
  "${GAIA_REPO}/demos/airport_q1/inc/airport_q1.fbs")

foreach(FBS_SOURCE ${FBS_SOURCES})
  get_filename_component(FBS_SOURCE_FILENAME ${FBS_SOURCE} NAME)
  string(REGEX REPLACE "\\.fbs$" "_builder.h" GEN_HEADER ${FBS_SOURCE_FILENAME})

  add_custom_command(
    OUTPUT "${GEN_DIR}/${GEN_HEADER}"
    COMMAND cmake -E make_directory "${GEN_DIR}"
    COMMAND "${FLATCC}" -a -o "${GEN_DIR}" "${FBS_SOURCE}"
    DEPENDS "${FBS_SOURCE}"
    COMMENT "Completed flatbuffers compilation of ${FBS_SOURCE}!"
    VERBATIM)

  add_custom_target(${FBS_SOURCE_FILENAME} ALL DEPENDS "${GEN_DIR}/${GEN_HEADER}")
endforeach(FBS_SOURCE)

# Compile config.

add_library(gaia_fdw SHARED
  type_mapping.cpp
  airport_demo_type_mapping.cpp
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

add_dependencies(gaia_fdw "airport_q1.fbs")

target_include_directories(gaia_fdw PRIVATE "${PROJECT_SOURCE_DIR}/inc")
target_include_directories(gaia_fdw PRIVATE "${GAIA_INC}/public/common")
target_include_directories(gaia_fdw PRIVATE "${GAIA_INC}/internal/common")
target_include_directories(gaia_fdw PRIVATE "${GAIA_INC}/public/db")
target_include_directories(gaia_fdw PRIVATE "${GAIA_INC}/internal/db")
target_include_directories(gaia_fdw SYSTEM PRIVATE "${GEN_DIR}")
target_include_directories(gaia_fdw SYSTEM PRIVATE "${PGSQL_INCLUDEDIR_SERVER}")

target_link_libraries(gaia_fdw PRIVATE flatccrt)
target_link_libraries(gaia_fdw PRIVATE gaia_se_client)

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
