#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.12)

# Set the project name.
project(addr_book_schema)

set(ADDR_BOOK_DDL "${PROJECT_SOURCE_DIR}/addr_book.ddl")

#
# Generate stand alone address_book.
#
add_custom_command(
  OUTPUT ${GAIA_GENERATED_SCHEMAS}/gaia_addr_book.h
  COMMAND ${GAIA_GENERATE} -t ${GAIA_PROD_BUILD}/db/storage_engine -o ${GAIA_GENERATED_SCHEMAS}
    ${ADDR_BOOK_DDL}
  DEPENDS gaiac
  DEPENDS ${ADDR_BOOK_DDL}
  COMMENT "Compiled addr_book.ddl."
)

add_custom_target(generate_addr_book_headers ALL
  DEPENDS ${GAIA_GENERATED_SCHEMAS}/gaia_addr_book.h
  # Add previous target to serialize parallel builds.
  DEPENDS generate_rules_headers
)

#
# Generate full "database" in one header by combining
# all schemas. Use a cmake script
# file to concatenate files together. Put this file under
# add_custom_command so that it correctly gets rebuilt if
# any of the dependent files change.
#
add_custom_command(
  OUTPUT ${GAIA_GENERATED_SCHEMAS}/gaia_addr_book_db.h
  COMMAND cmake
    -D GAIA_GENERATED_SCHEMAS=${GAIA_GENERATED_SCHEMAS}
    -D GAIA_USER_SCHEMA=${ADDR_BOOK_DDL}
    -D OUTPUT_DDL_PREFIX=${GAIA_GENERATED_SCHEMAS}/addr_book
    -P ${GAIA_SCHEMAS_INC}/build_db_ddl.cmake
  COMMAND ${GAIA_GENERATE} -t ${GAIA_PROD_BUILD}/db/storage_engine -o ${GAIA_GENERATED_SCHEMAS}
    ${GAIA_GENERATED_SCHEMAS}/addr_book_db.ddl
  DEPENDS gaiac
  DEPENDS ${ADDR_BOOK_DDL}
  COMMENT "Compiled addr_book_db.ddl."
)

add_custom_target(generate_addr_book_db_headers ALL
  DEPENDS ${GAIA_GENERATED_SCHEMAS}/gaia_addr_book_db.h
  # Add previous target to serialize parallel builds.
  DEPENDS generate_addr_book_headers
)

