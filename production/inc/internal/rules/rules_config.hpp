/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include "cpptoml.h"

namespace gaia
{
namespace rules
{

/**
 * Initializes the rules engine with configuration options specified in a
 * user-supplied gaia_conf.toml file.
 */
void initialize_rules_engine(shared_ptr<cpptoml::table>& rules_config);

} // namespace rules
} // namespace gaia
