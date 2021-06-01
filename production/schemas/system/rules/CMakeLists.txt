#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.12)

# Set the project name.
project(rules_schema)

string(RANDOM EVENT_LOG_INSTANCE_NAME)

add_custom_command(
  COMMENT "Generating event_log direct access APIs..."
  OUTPUT ${GAIA_GENERATED_SCHEMAS}/gaia_event_log.h
  COMMAND ${GAIA_GENERATE}
    -t ${GAIA_PROD_BUILD}/db/core
    -o ${GAIA_GENERATED_SCHEMAS}
    -g
    -n ${EVENT_LOG_INSTANCE_NAME}
    -d event_log
  DEPENDS gaiac
)

add_custom_target(generate_rules_headers ALL
  DEPENDS ${GAIA_GENERATED_SCHEMAS}/gaia_event_log.h
)
