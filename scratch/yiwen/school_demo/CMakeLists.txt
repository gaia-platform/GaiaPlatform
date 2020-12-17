#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.16)

# Set the project name.
project(school_demo)

set(CMAKE_CXX_STANDARD 17)
set(GAIA "/opt/gaia")
set(LOCAL "/usr/local")
set(GAIA_COMPILE_FLAGS "-c -Wall -Wextra -ggdb")
set(GENERATED_OUTPUTS "${CMAKE_BINARY_DIR}")
set(GAIA_INC "${GAIA}/include")

set(GAIA_SCHOOL_DEMO_INCLUDES
  ${GAIA_INC}/common
  ${GAIA_INC}/direct_access
  ${GAIA_INC}/system
  ${GAIA_INC}/common
  ${GAIA_INC}/rules
  ${GAIA_INC}/db
  ${GAIA_INC}
  ${GENERATED_OUTPUTS}
  ${PROJECT_SOURCE_DIR}
)

set(GAIA_GENERATE "${GAIA}/bin/gaiac")
set(GAIA_TRANSLATE "${GAIA}/bin/gaiat")

set(SCHOOL_DDL ${PROJECT_SOURCE_DIR}/school.ddl)
set(SCHOOL_RULESET ${PROJECT_SOURCE_DIR}/school.ruleset)
set(PHONE_RULESET ${PROJECT_SOURCE_DIR}/phone.ruleset)
file(MAKE_DIRECTORY ${GENERATED_OUTPUTS})

add_custom_command(
  OUTPUT ${GENERATED_OUTPUTS}/gaia_school.h
  COMMAND ${GAIA_GENERATE} -o ${GENERATED_OUTPUTS} -g ${SCHOOL_DDL}
  DEPENDS ${SCHOOL_DDL}
  COMMENT "Compiled school.ddl."
)

add_custom_target(generate_school_headers ALL
  DEPENDS ${GENERATED_OUTPUTS}/gaia_school.h
)
add_custom_command(
  OUTPUT ${PROJECT_BINARY_DIR}/school_rules.cpp
  COMMAND ${GAIA_TRANSLATE} ${SCHOOL_RULESET} -output ${PROJECT_BINARY_DIR}/school_rules.cpp -- -I /usr/lib/gcc/x86_64-linux-gnu/9/include/ -I ${GAIA_INC} -I ${PROJECT_SOURCE_DIR} -I ${GENERATED_OUTPUTS}
  DEPENDS ${SCHOOL_RULESET}
  COMMENT "Translating school.ruleset."
)

add_library(gphone SHARED
	    phone_lib.cpp)

add_dependencies(gphone generate_school_headers)

target_include_directories(gphone PRIVATE ${GAIA_SCHOOL_DEMO_INCLUDES})
set_target_properties(gphone PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")

add_custom_command(
  OUTPUT ${PROJECT_BINARY_DIR}/phone_rules.cpp
  COMMAND ${GAIA_TRANSLATE} ${PHONE_RULESET} -output ${PROJECT_BINARY_DIR}/phone_rules.cpp -- -I /usr/lib/gcc/x86_64-linux-gnu/9/include/ -I ${GAIA_INC} -I ${PROJECT_SOURCE_DIR} -I ${GENERATED_OUTPUTS} -lgphone
  DEPENDS ${PHONE_RULESET}
  DEPENDS gphone
  COMMENT "Translating phone.ruleset."
)

add_custom_target(translate_phone_ruleset ALL
  DEPENDS generate_school_headers
  DEPENDS gphone
  DEPENDS ${PROJECT_BINARY_DIR}/phone_rules.cpp
)

add_custom_target(translate_school_ruleset ALL
  DEPENDS generate_school_headers
  DEPENDS ${PROJECT_BINARY_DIR}/school_rules.cpp
)

add_library(gaia SHARED IMPORTED GLOBAL)
set_target_properties(gaia PROPERTIES IMPORTED_LOCATION ${LOCAL}/lib/libgaia.so)

add_executable(school 
    main.cpp
    school_sim.cpp
    ${PROJECT_BINARY_DIR}/school_rules.cpp
)

configure_file("${PROJECT_SOURCE_DIR}/gaia.conf" "${PROJECT_BINARY_DIR}/gaia.conf")
configure_file("${PROJECT_SOURCE_DIR}/gaia_log.conf" "${PROJECT_BINARY_DIR}/gaia_log.conf")

add_dependencies(school translate_school_ruleset translate_phone_ruleset)
set_target_properties(school PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
target_include_directories(school PRIVATE ${GAIA_SCHOOL_DEMO_INCLUDES})
target_link_libraries(school PRIVATE rt pthread gaia gphone)


#add_dependencies(messenger translate_phone_ruleset)
#set_target_properties(messenger PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")
#target_include_directories(messenger PRIVATE ${GAIA_SCHOOL_DEMO_INCLUDES})
#target_link_libraries(messenger PRIVATE rt pthread gaia gphone)
