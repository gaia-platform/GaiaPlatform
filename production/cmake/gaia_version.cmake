#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################


option(GIT_HEAD_HASH "Hash of the current commit.")
option(GIT_MASTER_HASH "Hash of the latests origin/master commit.")

# CI builds will not be able to produce the git hash because
# the source is copied without the Git info.
if (NOT GIT_HEAD_HASH)

    find_package(Git)

    # Get the latest git hash for the current branch.
    execute_process(
    COMMAND
        ${GIT_EXECUTABLE} rev-parse --short HEAD
    RESULT_VARIABLE
        STATUS
    OUTPUT_VARIABLE
        GIT_HEAD_HASH
    )

    if (NOT ${STATUS} EQUAL 0)
        set(GIT_HEAD_HASH "not-found")
    else()
        string(REGEX REPLACE "\n" "" GIT_HEAD_HASH "${GIT_HEAD_HASH}")
    endif()
endif()

if (NOT GIT_MASTER_HASH)

    find_package(Git)

    # Get the latest git hash from master merged into the current branch.
    execute_process(
    COMMAND
        ${GIT_EXECUTABLE} rev-parse --short origin/master
    RESULT_VARIABLE
        STATUS
    OUTPUT_VARIABLE
        GIT_MASTER_HASH
    )

    if (NOT ${STATUS} EQUAL 0)
        set(GIT_MASTER_HASH "not-found")
    else()
        string(REGEX REPLACE "\n" "" GIT_MASTER_HASH "${GIT_MASTER_HASH}")
    endif()
endif()

# Populate the gaia_version file. Note that this applies only for local builds.
# In CI the BUILD_NUMBER is replaced directly before calling cmake.
string(TIMESTAMP CURRENT_TIME "%Y%m%d%H%M%S")
set(BUILD_NUMBER "$ENV{USER}.${CURRENT_TIME}")

# If this is a CI build this value is set directly in the hpp.in file.
set(IS_CI_BUILD false)
configure_file(${GAIA_INC}/gaia_internal/common/gaia_version.hpp.in ${GAIA_INC}/gaia_internal/common/gaia_version.hpp)
