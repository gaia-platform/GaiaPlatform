#############################################
# Copyright (c) Gaia Platform LLC
# All rights reserved.
#############################################

# TeamCity/gdev builds will not be able to produce the git hash because
# the source are copied without the Git info.
set(GIT_HEAD_HASH "not-found")
set(GIT_MASTER_HASH "not-found")

# Populate the gaia_version file. Not this applies only for local builds.
# In CI the BUILD_NUMBER is replaced directly by TeamCity before calling cmake.
string(TIMESTAMP CURRENT_TIME "%Y%m%d%H%M%S")
set(BUILD_NUMBER "$ENV{USER}.${CURRENT_TIME}")

# If this is a CI build this value is set by TeamCity directly in the hpp.in file.
set(IS_CI_BUILD false)
configure_file(${GAIA_INC}/gaia_internal/common/gaia_version.hpp.in ${GAIA_INC}/gaia_internal/common/gaia_version.hpp)
