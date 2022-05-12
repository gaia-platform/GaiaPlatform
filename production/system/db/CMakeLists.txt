#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

project(system_db)

# Our main target.
add_library(gaia_system_db STATIC ../src/gaia_system.cpp)

configure_gaia_target(gaia_system_db)
target_include_directories(gaia_system_db PUBLIC ${GAIA_SYSTEM_PUBLIC_INCLUDES})
target_include_directories(gaia_system_db PRIVATE ${GAIA_SYSTEM_PRIVATE_INCLUDES})
target_link_libraries(gaia_system_db PUBLIC gaia_catalog gaia_direct)
