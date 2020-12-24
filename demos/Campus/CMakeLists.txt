#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

project(CampusDemo)
cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

find_package(Curses REQUIRED)
find_package(GTest REQUIRED)
find_package(spdlog REQUIRED)
#find_package(gaia REQUIRED)
#find_package(GMock REQUIRED)

set(GAIA_INC "/opt/gaia/include")                     #TODO: remove fixed path
set(GAIA_BIN "/opt/gaia/bin")                         #TODO: remove fixed path
set(GAIA_LIB "/opt/gaia/lib")                         #TODO: remove fixed path

find_library(MENU_LIBRARY menu)

# make sure we can find the ThingAPI package...
if(NOT TARGET ThingAPI::ThingAPI)
	if("$ENV{EDGE_SDK_HOME}" STREQUAL "")
	    message(FATAL_ERROR "Environment variable EDGE_SDK_HOME is not set. Please run config_env_variables.com/bat script to set the environment variables")
	endif()

    file(TO_CMAKE_PATH "$ENV{EDGE_SDK_HOME}" EDGE_SDK_HOME_PATH)
	list(APPEND CMAKE_MODULE_PATH "${EDGE_SDK_HOME_PATH}/cmake")

	find_package(ThingAPI REQUIRED)
endif()

include(GoogleTest)

set(GAIA_COMPILE_FLAGS "-c -Wall -Wextra -ggdb")

set(COMMON_SOURCE_DIR "${PROJECT_SOURCE_DIR}/common")
set(GENERATED_OUTPUTS "${COMMON_SOURCE_DIR}/generated")

set(GAIA_CAMPUS_DEMO_INCLUDES
  ${GTEST_INCLUDE_DIRS}
  ${CURSES_INCLUDE_DIR}
  #${GMOCK_INCLUDE_DIRS}
  ${GAIA_INC}
  ${GAIA_INC}/gaia
  ${GAIA_INC}/gaia/db
  ${GAIA_INC}/gaia/direct_access
  ${GAIA_INC}/gaia/rules
  ${FLATBUFFERS_INC}
  ${GENERATED_OUTPUTS}
)

set(GAIA_GENERATE ${GAIA_BIN}/gaiac)                             
set(GAIA_TRANSLATE ${GAIA_BIN}/gaiat)     

#add_compile_definitions(MESSAGE_BUS_MONOLITH)
add_compile_definitions(MESSAGE_BUS_EDGE)
#add_compile_definitions(MESSAGE_BUS_ROS1)
#add_compile_definitions(MESSAGE_BUS_ROS2)

# --- Generate ---

set(CAMPUS_DDL ${COMMON_SOURCE_DIR}/src/campus.ddl)
set(CAMPUS_RULESET ${COMMON_SOURCE_DIR}/src/campus.ruleset)
file(MAKE_DIRECTORY ${GENERATED_OUTPUTS})

add_custom_command(
  OUTPUT ${GENERATED_OUTPUTS}/gaia_campus.h
  #COMMAND '${GAIA_BIN}/gaia_se_server &'
  #COMMAND sleep 1
  COMMAND ${GAIA_GENERATE} -o ${GENERATED_OUTPUTS} -g ${CAMPUS_DDL}
  DEPENDS ${CAMPUS_DDL}
  COMMENT "Compiled campus.ddl."
)

add_custom_target(generate_campus_headers ALL
  DEPENDS ${GENERATED_OUTPUTS}/gaia_campus.h
)

#--- Translate ---

add_custom_command(
  OUTPUT ${GENERATED_OUTPUTS}/campus_rules.cpp
  #COMMAND ${GAIA_PROD_BUILD}/db/storage_engine/gaia_se_server &
  #COMMAND sleep 1
  COMMAND ${GAIA_TRANSLATE} ${CAMPUS_RULESET} -output ${GENERATED_OUTPUTS}/campus_rules.cpp -- -I /usr/lib/gcc/x86_64-linux-gnu/8/include/
  DEPENDS ${CAMPUS_RULESET}
  COMMENT "Translating campus.ruleset."
)

add_custom_target(translate_campus_ruleset ALL
  DEPENDS generate_campus_headers
  DEPENDS ${GENERATED_OUTPUTS}/campus_rules.cpp
)

#--- Done with D&T ---

add_library(gaia SHARED IMPORTED GLOBAL)
set_target_properties(gaia PROPERTIES IMPORTED_LOCATION ${GAIA_LIB}/libgaia.so) 

add_executable(campus 
    common/src/campus_demo.cpp
    common/src/campus_demo_term.cpp
    common/tests/test_campus_demo.cpp 
    ${GENERATED_OUTPUTS}/campus_rules.cpp    
    #edge/src/person_thing.cpp 
)

# The campus demo uses a system configuration and logger configuration file.
configure_file("${COMMON_SOURCE_DIR}/src/gaia_conf.toml" "${PROJECT_BINARY_DIR}/gaia_conf.toml")
configure_file("${COMMON_SOURCE_DIR}/src/log_conf.toml" "${PROJECT_BINARY_DIR}/log_conf.toml")


set_target_properties(campus PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")  
target_include_directories(campus PRIVATE ${GAIA_CAMPUS_DEMO_INCLUDES})
#1target_link_libraries(campus PRIVATE rt pthread gaia gaia_semock gaia_rules gaia_direct)

#set(SPDLOG_BUILD_SHARED OFF)
target_link_libraries(campus PUBLIC spdlog::spdlog ThingAPI::ThingAPI) 
target_link_libraries(campus PRIVATE ${GTEST_LIBRARIES} ${MENU_LIBRARY} ${CURSES_LIBRARIES} rt pthread gaia)

###########################################################################

add_custom_target(${PROJECT_NAME}_copy_config_files ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory config
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/edge/config/PersonProperties.json
        ${CMAKE_CURRENT_SOURCE_DIR}/edge/config/UIProperties.json
        ${CMAKE_CURRENT_SOURCE_DIR}/edge/config/CampusProperties.json
        config
)

add_custom_target(${PROJECT_NAME}_copy_definition_files ALL 
    COMMAND ${CMAKE_COMMAND} -E make_directory definitions/TagGroup/io.gaiaplatform.campus
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/edge/definitions/TagGroup/io.gaiaplatfom.campus/PersonTagGroup.json
        ${CMAKE_CURRENT_SOURCE_DIR}/edge/definitions/TagGroup/io.gaiaplatfom.campus/ActionTagGroup.json
        ${CMAKE_CURRENT_SOURCE_DIR}/edge/definitions/TagGroup/io.gaiaplatfom.campus/AlertTagGroup.json
        definitions/TagGroup/io.gaiaplatform.campus
    COMMAND ${CMAKE_COMMAND} -E make_directory definitions/ThingClass/io.gaiaplatform.campus
    COMMAND ${CMAKE_COMMAND} -E copy_if_different      
        ${CMAKE_CURRENT_SOURCE_DIR}/edge/definitions/ThingClass/io.gaiaplatform.campus/PersonThingClass.json
        ${CMAKE_CURRENT_SOURCE_DIR}/edge/definitions/ThingClass/io.gaiaplatform.campus/CampusThingClass.json
        ${CMAKE_CURRENT_SOURCE_DIR}/edge/definitions/ThingClass/io.gaiaplatform.campus/UIThingClass.json
        definitions/ThingClass/io.gaiaplatform.campus
)


