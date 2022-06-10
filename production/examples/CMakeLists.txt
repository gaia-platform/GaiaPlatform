###################################################
# Copyright (c) Gaia Platform Authors
#
# Use of this source code is governed by the MIT
# license that can be found in the LICENSE.txt file
# or at https://opensource.org/licenses/MIT.
###################################################

project(examples)

# To keep things simple we don't handle the default database yet.
# Anyway this is not a customer visible example.
#
#add_example(
#  NAME default
#  DDL_FILE ${PROJECT_SOURCE_DIR}/default/hello.ddl
#  DB_NAME ""
#  RULESET_FILE ${PROJECT_SOURCE_DIR}/default/hello.ruleset
#  SRC_FILES default/hello.cpp
#)

add_example(
  NAME direct_access
  DDL_FILE ${PROJECT_SOURCE_DIR}/direct_access/hospital.ddl
  DB_NAME hospital
  SRC_FILES direct_access/hospital.cpp
)

add_example(
  NAME direct_access_multithread
  DDL_FILE ${PROJECT_SOURCE_DIR}/direct_access_multithread/counter.ddl
  DB_NAME counter
  SRC_FILES direct_access_multithread/counter.cpp
)

add_example(
  NAME direct_access_vlr
  DDL_FILE ${PROJECT_SOURCE_DIR}/direct_access_vlr/direct_access_vlr.ddl
  DB_NAME direct_access_vlr
  SRC_FILES direct_access_vlr/direct_access_vlr.cpp
)

# Examples that use a ruleset need gaiat.
if(BUILD_GAIA_SDK)
  add_example(
    NAME hello
    DDL_FILE ${PROJECT_SOURCE_DIR}/hello/hello.ddl
    DB_NAME hello
    RULESET_FILE ${PROJECT_SOURCE_DIR}/hello/hello.ruleset
    SRC_FILES hello/hello.cpp
  )

  add_example(
    NAME incubator
    DDL_FILE ${PROJECT_SOURCE_DIR}/incubator/incubator.ddl
    DB_NAME incubator
    RULESET_FILE ${PROJECT_SOURCE_DIR}/incubator/incubator.ruleset
    SRC_FILES incubator/incubator.cpp
  )

  add_example(
    NAME rules
    DDL_FILE ${PROJECT_SOURCE_DIR}/rules/clinic.ddl
    DB_NAME clinic
    RULESET_FILE ${PROJECT_SOURCE_DIR}/rules/clinic.ruleset
    SRC_FILES "rules/clinic.cpp;rules/lesson_manager.cpp"
    INC_DIRS ${PROJECT_SOURCE_DIR}/rules
  )

  add_example(
    NAME rules_vlr
    DDL_FILE ${PROJECT_SOURCE_DIR}/rules_vlr/rules_vlr.ddl
    DB_NAME rules_vlr
    RULESET_FILE ${PROJECT_SOURCE_DIR}/rules_vlr/rules_vlr.ruleset
    SRC_FILES rules_vlr/rules_vlr.cpp
  )
endif()
