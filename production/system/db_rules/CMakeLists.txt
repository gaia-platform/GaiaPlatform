#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

project(system_db_rules)

# Conditionally compile in the rules init and shutdown functions.
add_compile_definitions(INCLUDE_RULES_ENGINE=1)

# Our main target.
add_library(gaia_system STATIC ../src/gaia_system.cpp)

# gaia_system includes the database and rules engine
configure_gaia_target(gaia_system)
target_include_directories(gaia_system PUBLIC ${GAIA_SYSTEM_PUBLIC_INCLUDES})
target_include_directories(gaia_system PRIVATE ${GAIA_SYSTEM_PRIVATE_INCLUDES})
target_link_libraries(gaia_system PUBLIC gaia_catalog gaia_direct gaia_rules)
