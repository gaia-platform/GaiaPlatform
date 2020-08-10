#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.12)

# Set the project name.
project(rules_schema)

add_custom_command(
  OUTPUT ${GAIA_GENERATED_SCHEMAS}/gaia_event_log.h
  COMMAND ${GAIA_GENERATE} -t ${GAIA_PROD_BUILD}/db/storage_engine -o ${GAIA_GENERATED_SCHEMAS} ${EVENT_LOG_DDL}
  DEPENDS gaiac
  DEPENDS ${EVENT_LOG_DDL}
  COMMENT "Compiled ${EVENT_LOG_DDL}"
)

add_custom_target(generate_rules_headers ALL
  DEPENDS ${GAIA_GENERATED_SCHEMAS}/gaia_event_log.h
)
