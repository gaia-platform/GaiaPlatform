#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

cmake_minimum_required(VERSION 3.11)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

# Set the project name.
project(campus_demo)

find_package(Curses REQUIRED)
find_package(GTest REQUIRED)
find_package(spdlog REQUIRED)
#find_package(gaia REQUIRED)
#find_package(GMock REQUIRED)

set(GAIA_INC "/usr/local/include/gaia")                                                     #TODO: remove fixed path
set(GAIA_BIN "/usr/local/bin")                                                              #TODO: remove fixed path
set(GAIA_PROD_BUILD "/media/mark/Data1/U20/develop/GaiaPlatform/build/production")          #TODO: remove fixed path
set(GAIA_PROD_INC "/media/mark/Data1/U20/develop/GaiaPlatform/production/inc/public")       #TODO: remove fixed path

set(GAIA_PROD "${GAIA_REPO}/production")                                                    #TODO: We should not have to refer to anything internal
set(GAIA_INC "/media/mark/Data1/U20/develop/GaiaPlatform/production/inc/public")            #TODO: We should not have to refer to anything internal
set(GAIA_INC_INTERNAL "/media/mark/Data1/U20/develop/GaiaPlatform/production/inc/internal") #TODO: We should not have to refer to anything internal

find_library(MENU_LIBRARY menu)

include(GoogleTest)

set(GAIA_COMPILE_FLAGS "-c -Wall -Wextra -ggdb")
#!set(GENERATED_OUTPUTS "${CMAKE_BINARY_DIR}/campus_demo/generated")
set(GENERATED_OUTPUTS "${PROJECT_SOURCE_DIR}/generated")

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
  ${GAIA_INC_INTERNAL}/common #TODO: We should not have to refer to anything internal
  ${GAIA_INC_INTERNAL}/rules  #TODO: We should not have to refer to anything internal
  ${GAIA_INC_INTERNAL}/db     #TODO: We should not have to refer to anything internal
)

set(GAIA_GENERATE ${GAIA_BIN}/gaiac)                             
#set(GAIA_GENERATE ${GAIA_PROD_BUILD}/catalog/gaiac/gaiac)         
set(GAIA_TRANSLATE ${GAIA_BIN}/gaiat)                             
#set(GAIA_TRANSLATE ${GAIA_PROD_BUILD}/tools/gaia_translate/gaiat)

# --- Generate ---

set(INCUBATOR_DDL ${PROJECT_SOURCE_DIR}/src/incubator.ddl)
set(INCUBATOR_RULESET ${PROJECT_SOURCE_DIR}/src/incubator.ruleset)
file(MAKE_DIRECTORY ${GENERATED_OUTPUTS})

add_custom_command(
  OUTPUT ${GENERATED_OUTPUTS}/gaia_incubator.h
  #COMMAND '${GAIA_BIN}/gaia_se_server &'
  #COMMAND sleep 1
  COMMAND ${GAIA_GENERATE} -o ${GENERATED_OUTPUTS} -g ${INCUBATOR_DDL}
  DEPENDS ${INCUBATOR_DDL}
  COMMENT "Compiled incubator.ddl."
)

add_custom_target(generate_incubator_headers ALL
  DEPENDS ${GENERATED_OUTPUTS}/gaia_incubator.h
)

#--- Translate ---

add_custom_command(
  OUTPUT ${PROJECT_SOURCE_DIR}/incubator_rules.cpp
  #COMMAND ${GAIA_PROD_BUILD}/db/storage_engine/gaia_se_server &
  #COMMAND sleep 1
  COMMAND ${GAIA_TRANSLATE} ${INCUBATOR_RULESET} -output ${PROJECT_SOURCE_DIR}/incubator_rules.cpp -- -I /usr/lib/gcc/x86_64-linux-gnu/8/include/
  DEPENDS ${INCUBATOR_RULESET}
  COMMENT "Translating incubator.ruleset."
)

# disable for now: Exception while processing the catalog std::exception
add_custom_target(translate_incubator_ruleset ALL
  DEPENDS generate_incubator_headers
  DEPENDS ${PROJECT_SOURCE_DIR}/incubator_rules.cpp
)

add_library(gaia SHARED IMPORTED GLOBAL)
set_target_properties(gaia PROPERTIES IMPORTED_LOCATION /usr/local/lib/libgaia.so) #TODO: remove fixed path #TODO: Temporarily point to prod instead
#set_target_properties(gaia PROPERTIES IMPORTED_LOCATION ${GAIA_PROD_BUILD}/sdk/libgaia.so) #can't, file not found

add_executable(campus 
    src/main.cpp
    src/rules.cpp
    src/campus_demo.cpp
    src/campus_demo_term.cpp
    tests/test_campus_demo.cpp
    incubator_rules.cpp        
)

# The incubator demo uses a system configuration and logger configuration file.
configure_file("${PROJECT_SOURCE_DIR}/src/gaia_conf.toml" "${PROJECT_BINARY_DIR}/gaia_conf.toml")
configure_file("${PROJECT_SOURCE_DIR}/src/log_conf.toml" "${PROJECT_BINARY_DIR}/log_conf.toml")

#!add_dependencies(campus translate_incubator_ruleset)
set_target_properties(campus PROPERTIES COMPILE_FLAGS "${GAIA_COMPILE_FLAGS}")  
target_include_directories(campus PRIVATE ${GAIA_CAMPUS_DEMO_INCLUDES})
#1target_link_libraries(campus PRIVATE rt pthread gaia gaia_semock gaia_rules gaia_direct)

#set(SPDLOG_BUILD_SHARED OFF)
target_link_libraries(campus PUBLIC spdlog::spdlog) 
target_link_libraries(campus PRIVATE ${GTEST_LIBRARIES} ${MENU_LIBRARY} ${CURSES_LIBRARIES} rt pthread gaia)

