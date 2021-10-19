#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# Set the project name.
project(test_schemas)

process_schema_internal(
    DDL_FILE "${PROJECT_SOURCE_DIR}/addr_book/addr_book.ddl"
    DATABASE_NAME "addr_book")
process_schema_internal(
    DDL_FILE "${PROJECT_SOURCE_DIR}/airport/airport.ddl"
    DATABASE_NAME "airport")
process_schema_internal(
    DDL_FILE "${PROJECT_SOURCE_DIR}/prerequisites/prerequisites.ddl"
    DATABASE_NAME "prerequisites")
process_schema_internal(
    DDL_FILE "${PROJECT_SOURCE_DIR}/index_sandbox/index_sandbox.ddl"
    DATABASE_NAME "index_sandbox")
process_schema_internal(
    DDL_FILE "${PROJECT_SOURCE_DIR}/one_to_one/one_to_one.ddl"
    DATABASE_NAME "one_to_one")
process_schema_internal(
    DDL_FILE "${PROJECT_SOURCE_DIR}/incubator/incubator.ddl"
    DATABASE_NAME barn_storage)
process_schema_internal(
    DDL_FILE "${PROJECT_SOURCE_DIR}/amr_swarm/amr_swarm.ddl"
    DATABASE_NAME amr_swarm)

process_schema_internal(DDL_FILE "${PROJECT_SOURCE_DIR}/default/no_db.ddl")
