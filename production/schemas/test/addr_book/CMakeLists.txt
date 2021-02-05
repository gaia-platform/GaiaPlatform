#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(addr_book_schema)

set(ADDR_BOOK_DDL "${PROJECT_SOURCE_DIR}/addr_book.ddl")

#
# Generate stand alone address_book.
#
add_custom_command(
  COMMENT "Compiling addr_book.ddl..."
  OUTPUT ${GAIA_GENERATED_SCHEMAS}/gaia_addr_book.h
  COMMAND ${GAIA_GENERATE} --destroy-db -t ${GAIA_PROD_BUILD}/db/core -o ${GAIA_GENERATED_SCHEMAS} -g ${ADDR_BOOK_DDL}
  DEPENDS gaiac
  DEPENDS ${ADDR_BOOK_DDL}
)

add_custom_target(generate_addr_book_headers ALL
  DEPENDS ${GAIA_GENERATED_SCHEMAS}/gaia_addr_book.h
  # Add previous target to serialize parallel builds.
  DEPENDS generate_rules_headers
)
