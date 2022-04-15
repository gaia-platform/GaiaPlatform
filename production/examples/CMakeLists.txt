#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

project(examples)

add_example(
  NAME hello
  DDL_FILE ${PROJECT_SOURCE_DIR}/hello/hello.ddl
  DB_NAME hello
  RULESET_FILE ${PROJECT_SOURCE_DIR}/hello/hello.ruleset
  SOURCE_FILES hello/hello.cpp
)

add_example(
  NAME direct_access
  DDL_FILE ${PROJECT_SOURCE_DIR}/direct_access/hospital.ddl
  DB_NAME hospital
  SOURCE_FILES ${PROJECT_SOURCE_DIR}/direct_access/hospital.cpp
)
