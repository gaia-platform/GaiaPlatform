#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.12)

# Set the project name.
project(rules_schema)

string(RANDOM EVENT_LOG_INSTANCE_NAME)

process_schema_internal(DATABASE_NAME event_log)
