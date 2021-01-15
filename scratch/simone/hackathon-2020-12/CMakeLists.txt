cmake_minimum_required(VERSION 3.16)

project(gaia_demo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

enable_testing()
include(/opt/gaia/cmake/gaia.cmake)

# Headers
set(GAIA_INC "/opt/gaia/include")
set(GAIA_LIB "/opt/gaia/lib")

set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${GAIA_LIB})

# Sources
set(SCHEMAS_DIR ${PROJECT_SOURCE_DIR}/schema)
set(RULES_DIR ${PROJECT_SOURCE_DIR}/rules)
set(GAIA_GENERATED_OUTPUTS "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${GAIA_GENERATED_OUTPUTS})

set(GAIA_DEMO_INCLUDES
  ${GAIA_INC}
  ${GAIA_GENERATED_OUTPUTS}
  ${PROJECT_SOURCE_DIR}/src
)

# --- Generate EDC from DDL---
generate_schema_headers(
  DDL_FILE ${SCHEMAS_DIR}/school.ddl
  OUTPUT_FOLDER ${GAIA_GENERATED_OUTPUTS}
  TARGET_NAME generate_school_headers)

# -- Translate ruleset in CPP --
translate_ruleset(
  RULESET_FILE ${RULES_DIR}/school.ruleset
  OUTPUT_FOLDER ${GAIA_GENERATED_OUTPUTS}
  TARGET_NAME translate_school_ruleset
  GAIAT_PARAMS -I ${PROJECT_SOURCE_DIR}/src
  DEPENDS generate_school_headers)

file(GLOB GAIA_GENERATED_SOURCES
  "${GAIA_GENERATED_OUTPUTS}/*.cpp"
)

add_executable(gaia_demo
  src/main.cpp
  ${GAIA_GENERATED_SOURCES}
)

add_dependencies(gaia_demo generate_school_headers)
add_dependencies(gaia_demo translate_school_ruleset)

target_include_directories(gaia_demo PRIVATE ${GAIA_DEMO_INCLUDES})
target_link_libraries(gaia_demo PRIVATE rt pthread gaia)

set(Boost_NO_BOOST_CMAKE ON)
find_package(Boost 1.55 REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

target_link_libraries(gaia_demo PRIVATE Boost::system Threads::Threads)
