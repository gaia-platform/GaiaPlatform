#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

project(S1_ConnectSensor)
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

set(GAIA_INC "/usr/local/include/gaia")                                                     #TODO: remove fixed path
set(GAIA_BIN "/usr/local/bin")                                                              #TODO: remove fixed path

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
  ${GAIA_INC}/catalog
  ${GAIA_INC}/common
  ${GAIA_INC}/db
  ${GAIA_INC}/direct_access
  ${GAIA_INC}/rules
  ${GAIA_INC}/system
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
set_target_properties(gaia PROPERTIES IMPORTED_LOCATION /usr/local/lib/libgaia.so) #TODO: remove fixed path 

add_executable(campus 
    common/src/campus_demo.cpp
    common/src/campus_demo_term.cpp
    common/tests/test_campus_demo.cpp 
    ${GENERATED_OUTPUTS}/campus_rules.cpp     
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

#if(NOT CMAKE_BUILD_TYPE)
#  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
#      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
#      FORCE)
#endif(NOT CMAKE_BUILD_TYPE)

add_executable(s1_temperaturesensor
    edge/src/TemperatureSensor.cpp
)

add_executable(s1_temperaturedisplay
    edge/src/TemperatureDisplay.cpp
)

target_link_libraries(s1_temperaturesensor
    ThingAPI::ThingAPI
)

target_link_libraries(s1_temperaturedisplay
    ThingAPI::ThingAPI
)

set_property(TARGET s1_temperaturesensor PROPERTY CXX_STANDARD 11)
set_property(TARGET s1_temperaturesensor PROPERTY OUTPUT_NAME "temperaturesensor")

set_property(TARGET s1_temperaturedisplay PROPERTY CXX_STANDARD 11)
set_property(TARGET s1_temperaturedisplay PROPERTY OUTPUT_NAME "temperaturedisplay")

add_custom_target(${PROJECT_NAME}_copy_config_files ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory config
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/edge/config/TemperatureDisplayProperties.json
        ${CMAKE_CURRENT_SOURCE_DIR}/edge/config/TemperatureSensorProperties.json
        config

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/edge/config/PersonProperties.json
        ${CMAKE_CURRENT_SOURCE_DIR}/edge/config/UIProperties.json
        config
)

add_custom_target(${PROJECT_NAME}_copy_definition_files ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory definitions/TagGroup/com.adlinktech.example
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/edge/definitions/TagGroup/com.adlinktech.example/TemperatureTagGroup.json
        definitions/TagGroup/com.adlinktech.example
    COMMAND ${CMAKE_COMMAND} -E make_directory definitions/ThingClass/com.adlinktech.example
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/edge/definitions/ThingClass/com.adlinktech.example/TemperatureDisplayThingClass.json
        ${CMAKE_CURRENT_SOURCE_DIR}/edge/definitions/ThingClass/com.adlinktech.example/TemperatureSensorThingClass.json 
        definitions/ThingClass/com.adlinktech.example 
   
    COMMAND ${CMAKE_COMMAND} -E make_directory definitions/TagGroup/io.gaiaplatform.campus
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/edge/definitions/TagGroup/io.gaiaplatfom.campus/PersonTagGroup.json
        definitions/TagGroup/io.gaiaplatform.campus
    COMMAND ${CMAKE_COMMAND} -E make_directory definitions/ThingClass/io.gaiaplatform.campus
    COMMAND ${CMAKE_COMMAND} -E copy_if_different      
        ${CMAKE_CURRENT_SOURCE_DIR}/edge/definitions/ThingClass/io.gaiaplatform.campus/PersonThingClass.json
        ${CMAKE_CURRENT_SOURCE_DIR}/edge/definitions/ThingClass/io.gaiaplatform.campus/UIThingClass.json
        definitions/ThingClass/io.gaiaplatform.campus
)

configure_file(edge/start_display.sh start_display.sh COPYONLY)
configure_file(edge/start_sensor.sh start_sensor.sh COPYONLY)

configure_file(edge/start_display.bat start_display.bat COPYONLY)
configure_file(edge/start_sensor.bat start_sensor.bat COPYONLY)

