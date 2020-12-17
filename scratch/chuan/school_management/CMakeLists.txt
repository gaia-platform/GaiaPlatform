cmake_minimum_required(VERSION 3.16)

project(school_management)

set(CMAKE_CXX_STANDARD 17)
set(GAIA "/opt/gaia")
set(GAIA_GENERATE "${GAIA}/bin/gaiac")
set(GAIA_TRANSLATE "${GAIA}/bin/gaiat")
set(GAIA_INC "${GAIA}/include")

add_library(gaia SHARED IMPORTED GLOBAL)
set_target_properties(gaia PROPERTIES IMPORTED_LOCATION ${GAIA}/lib/libgaia.so)

set(SCHOOL_RULESET ${PROJECT_SOURCE_DIR}/ruleset/school.ruleset)
set(SCHOOL_RULESET_SOURCE ${PROJECT_SOURCE_DIR}/src/school_ruleset.cpp)

set(SCHOOL_DDL ${PROJECT_SOURCE_DIR}/ddl/school.ddl)

add_custom_command(
  OUTPUT ${GAIA_INC}/gaia_school.h
  COMMAND ${GAIA_GENERATE} -o "${PROJECT_SOURCE_DIR}/include" -g ${SCHOOL_DDL}
  DEPENDS ${SCHOOL_DDL}
  COMMENT "Compiling ddl..."
)

add_custom_target(generate_headers ALL
  DEPENDS ${GAIA_INC}/gaia_school.h
)

add_custom_command(
  OUTPUT ${SCHOOL_RULESET_SOURCE}
  COMMAND ${GAIA_TRANSLATE} ${SCHOOL_RULESET} -output ${SCHOOL_RULESET_SOURCE} -- -I/usr/lib/gcc/x86_64-pc-linux-gnu/10.2.0/include
  DEPENDS ${SCHOOL_RULESET}
  COMMENT "Translating ruleset..."
)

add_custom_target(translate_school_ruleset ALL
  DEPENDS generate_headers
  DEPENDS ${SCHOOL_RULESET_SOURCE}
)

find_package(Pistache REQUIRED)

file(GLOB API_SOURCE
  ${PROJECT_SOURCE_DIR}/src/api/*.cpp
  ${PROJECT_SOURCE_DIR}/src/impl/*.cpp
  ${PROJECT_SOURCE_DIR}/src/model/*.cpp
)

add_executable(school_management
  ${PROJECT_SOURCE_DIR}/src/main.cpp
  ${PROJECT_SOURCE_DIR}/src/school_ruleset.cpp
  ${API_SOURCE}
)

add_dependencies(school_management translate_school_ruleset)
target_include_directories(school_management PRIVATE "${GAIA_INC};${PROJECT_SOURCE_DIR}/include")
target_link_libraries(school_management PRIVATE rt pthread gaia pistache_shared)
