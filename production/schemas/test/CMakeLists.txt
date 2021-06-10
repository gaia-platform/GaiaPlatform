#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(test_schemas)

process_schema_internal(DDL_FILE "${PROJECT_SOURCE_DIR}/addr_book/addr_book.ddl")
process_schema_internal(DDL_FILE "${PROJECT_SOURCE_DIR}/airport/airport.ddl")
process_schema_internal(DDL_FILE "${PROJECT_SOURCE_DIR}/prerequisites/prerequisites.ddl")
process_schema_internal(DDL_FILE "${PROJECT_SOURCE_DIR}/index_sandbox/index_sandbox.ddl")
process_schema_internal(
    DDL_FILE "${PROJECT_SOURCE_DIR}/incubator/incubator.ddl"
    DATABASE_NAME barn_storage)
