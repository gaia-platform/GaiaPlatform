/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/common/gaia_version.hpp"

namespace gaia
{
namespace common
{

std::string gaia_version()
{
    std::string version = std::to_string(c_gaia_version_major)
        + "." + std::to_string(c_gaia_version_minor)
        + "." + std::to_string(c_gaia_version_patch);

    if (strlen(c_gaia_pre_release) > 0)
    {
        version += "-" + std::string(c_gaia_pre_release);
    }

    return version;
}

std::string gaia_full_version(bool include_git)
{
    std::string version = gaia_version()
        + "+" + std::string(c_gaia_build_number);

    // TODO: ATM this is never true, we need to decide how/when to expose the git hash.
    if (include_git)
    {
        if (strcmp(c_gaia_local_git_hash, c_gaia_master_git_hash) == 0)
        {
            version += "(" + std::string(c_gaia_master_git_hash) + ")";
        }
        else
        {
            version += " (master:" + std::string(c_gaia_master_git_hash) + " local:" + std::string(c_gaia_local_git_hash) + ")";
        }
    }

    return version;
}

} // namespace common
} // namespace gaia
