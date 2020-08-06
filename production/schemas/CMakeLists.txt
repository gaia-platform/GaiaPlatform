#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.12)

# Set the project name.
project(schemas)

set(GENERATED_OUTPUTS "${CMAKE_BINARY_DIR}/schemas/generated")
set(GAIA_GENERATE ${GAIA_PROD_BUILD}/catalog/gaiac/gaiac)

#
# We are intentionally making the schema generation serialized because the
# gaiac tool used to generate the headers relies on the database to run.
# Since we currently don't support running multiple instances of the database at 
# the same time, we generate all schemas here in order.
#

#
# System DDL files we are pulling in.
#
set(CATALOG_DDL ${GAIA_REPO}/production/catalog/catalog_manager/src/catalog.ddl)
set(EVENT_LOG_DDL ${GAIA_REPO}/production/rules/event_manager/schema/event_log.ddl)

#
# Generate catalog headers.
#
add_custom_command(
  OUTPUT ${GENERATED_OUTPUTS}/gaia_catalog.h
  COMMAND cmake -E make_directory "${GENERATED_OUTPUTS}"
  COMMAND ${GAIA_GENERATE} -t ${GAIA_PROD_BUILD}/db/storage_engine -o ${GENERATED_OUTPUTS} ${CATALOG_DDL}
  DEPENDS ${GAIA_GENERATE}
  DEPENDS ${CATALOG_DDL}
  COMMENT "Compiled ${CATALOG_DDL}"
)

add_custom_target(generate_catalog_headers ALL
  DEPENDS ${GENERATED_OUTPUTS}/gaia_catalog.h
)

#
# Generate event_log headers.
#
add_custom_command(
  OUTPUT ${GENERATED_OUTPUTS}/gaia_event_log.h
  COMMAND ${GAIA_GENERATE} -t ${GAIA_PROD_BUILD}/db/storage_engine -o ${GENERATED_OUTPUTS} ${EVENT_LOG_DDL}
  DEPENDS ${GAIA_GENERATE}
  DEPENDS ${EVENT_LOG_DDL}
  COMMENT "${EVENT_LOG_DDL}"
)

add_custom_target(generate_event_log_headers ALL
  DEPENDS ${GENERATED_OUTPUTS}/gaia_event_log.h
  # Add previous target to serialize parallel builds.
  DEPENDS generate_catalog_headers
)

#
# Generate stand alone address_book.
#
add_custom_command(
  OUTPUT ${GENERATED_OUTPUTS}/gaia_addr_book.h
  COMMAND ${GAIA_GENERATE} -t ${GAIA_PROD_BUILD}/db/storage_engine -o ${GENERATED_OUTPUTS}
    ${GAIA_SCHEMAS}/addr_book.ddl
  DEPENDS ${GAIA_GENERATE}
  DEPENDS ${GAIA_SCHEMAS}/addr_book.ddl
  COMMENT "Compiled addr_book.ddl."
)

add_custom_target(generate_addr_book_headers ALL
  DEPENDS ${GENERATED_OUTPUTS}/gaia_addr_book.h
  # Add previous target to serialize parallel builds.
  DEPENDS generate_event_log_headers
)

#
# Generate full "database" in one header by combining
# catalog, event_log, and addr_book.  Use a cmake script
# file to concatenate files together.  Put this file under
# add_custom_command so that it correctly gets rebuilt if
# any of the dependent files change.
#
add_custom_command(
  OUTPUT ${GENERATED_OUTPUTS}/gaia_addr_book_db.h
  COMMAND cmake
    -D GENERATED_OUTPUTS=${GENERATED_OUTPUTS}
    -D CATALOG_DDL=${CATALOG_DDL} 
    -D EVENT_LOG_DDL=${EVENT_LOG_DDL}
    -D GAIA_USER_SCHEMA=${GAIA_SCHEMAS}/addr_book.ddl
    -D OUTPUT_DDL_PREFIX=${GENERATED_OUTPUTS}/addr_book
    -P ${GAIA_SCHEMAS}/build_db_ddl.cmake
  COMMAND ${GAIA_GENERATE} -t ${GAIA_PROD_BUILD}/db/storage_engine -o ${GENERATED_OUTPUTS}
    ${GENERATED_OUTPUTS}/addr_book_db.ddl
  DEPENDS ${GAIA_GENERATE}
  DEPENDS ${CATALOG_DDL}
  DEPENDS ${EVENT_LOG_DDL}
  DEPENDS ${GAIA_SCHEMAS}/addr_book.ddl
  COMMENT "Compiled addr_book_db.ddl."
)

add_custom_target(generate_addr_book_db_headers ALL
  DEPENDS ${GENERATED_OUTPUTS}/gaia_addr_book_db.h
  # Add previous target to serialize parallel builds.
  DEPENDS generate_addr_book_headers
)
