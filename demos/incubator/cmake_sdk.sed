# This file is used to transform our local build CMakeLists file into one appropriate
# for the SDK.  The production/sdk cmake file will configure this file with the correct
# values for GAIA_INC, GAIA_LIB, GAIA_GENERATE, and GAIA_TRANSLATE. After this sed file
# is configured, it is run against the CMakeLists.txt file which is then used in the
# incubator demo install.
#
# Add CMake version required
/# {SDK-SETUP:CMAKE}/ c cmake_minimum_required(VERSION 3.16)
# Add C++ standard required
/# {SDK-SETUP:CXX_STANDARD}/ c set(CMAKE_CXX_STANDARD 17)
# Insert SDK paths for include, lib, and tools.
/# {SDK-SETUP:VARS}/ i set(GAIA_INC "${GAIA_INC}")
/# {SDK-SETUP:VARS}/ i set(GAIA_LIB "${GAIA_LIB}")
/# {SDK-SETUP:VARS}/ i set(GAIA_GENERATE "${GAIA_GENERATE}")
/# {SDK-SETUP:VARS}/ i set(GAIA_TRANSLATE "${GAIA_TRANSLATE}")
# Remove our marker in the cmake file
/# {SDK-SETUP:VARS}/ d
