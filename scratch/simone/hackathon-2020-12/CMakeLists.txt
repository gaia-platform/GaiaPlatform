cmake_minimum_required(VERSION 3.16)

project(gaia_demo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

enable_testing()

# Headers
set(GAIA_INC "/opt/gaia/include")
set(GAIA_LIB "/opt/gaia/lib")

set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${GAIA_LIB})

# Binaries
set(GAIA_BIN "/usr/local/bin")
set(GAIA_GENERATE ${GAIA_BIN}/gaiac)
set(GAIA_TRANSLATE ${GAIA_BIN}/gaiat)

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
file(GLOB DDL_FILES "${SCHEMAS_DIR}/*.ddl")

foreach(DDL_FILE ${DDL_FILES})
    get_filename_component(DDL_NAME ${DDL_FILE} NAME)
    string(REPLACE ".ddl" "" DDL_NAME ${DDL_NAME})

    message(STATUS "Adding target to compile schema: ${DDL_FILE}")

    add_custom_command(
        COMMENT "Compiling ${DDL_FILE} into ${DDL_NAME}.h"
        OUTPUT  ${GAIA_GENERATED_OUTPUTS}/${DDL_NAME}.h
        COMMAND ${GAIA_GENERATE} -o ${GAIA_GENERATED_OUTPUTS} -g ${DDL_FILE}
        DEPENDS ${DDL_FILE}
    )

    add_custom_target(generate_${DDL_NAME}_headers ALL
        DEPENDS ${GAIA_GENERATED_OUTPUTS}/${DDL_NAME}.h
    )

    list(APPEND DDL_HEADERS_TARGETS generate_${DDL_NAME}_headers)
endforeach()

# -- Translate --
file(GLOB RULESET_FILES "${RULES_DIR}/*.ruleset")

foreach(RULESET_FILE ${RULESET_FILES})
    get_filename_component(RULESET_NAME ${RULESET_FILE} NAME)
    string(REPLACE ".ruleset" "" RULESET_NAME ${RULESET_NAME})
    set(RULESET_CPP_NAME ${RULESET_NAME}.cpp)
    set(RULESET_CPP_PATH ${GAIA_GENERATED_OUTPUTS}/${RULESET_CPP_NAME})

    message(STATUS "Adding target to translate ruleset: ${RULESET_FILE} into ${RULESET_CPP_NAME}")

    add_custom_command(
        COMMENT "Translating ${RULESET_FILE} into ${RULESET_CPP_NAME}"
        OUTPUT ${RULESET_CPP_PATH}
        COMMAND ${GAIA_TRANSLATE} ${RULESET_FILE} -output ${RULESET_CPP_PATH} --
            -I /usr/lib/gcc/x86_64-linux-gnu/9/include/
            -I ${GAIA_INC} -I ${GAIA_GENERATED_OUTPUTS}
            -I ${PROJECT_SOURCE_DIR}/src
            -std=c++17
        DEPENDS ${RULESET_FILE}
    )

    set(COMPILE_RULESET_TARGET generate_${RULESET_NAME}_rules)

    add_custom_target(${COMPILE_RULESET_TARGET} ALL
        DEPENDS ${RULESET_CPP_PATH}
    )

    list(APPEND RULES_TARGETS ${COMPILE_RULESET_TARGET})
    list(APPEND RULES_SOURCES ${RULESET_CPP_PATH})
endforeach()

message(STATUS "Compile DDL Targets: ${DDL_HEADERS_TARGETS}")
message(STATUS "Translate Rules targets: ${RULES_TARGETS}")

add_executable(gaia_demo
    src/main.cpp
    ${RULES_SOURCES}
)
add_dependencies(gaia_demo ${DDL_HEADERS_TARGETS})
add_dependencies(gaia_demo ${RULES_TARGETS})
target_include_directories(gaia_demo PRIVATE ${GAIA_DEMO_INCLUDES})
target_link_libraries(gaia_demo PRIVATE rt pthread gaia)


set(Boost_NO_BOOST_CMAKE ON)
find_package(Boost 1.55 REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

target_link_libraries(gaia_demo PRIVATE Boost::system Threads::Threads)
