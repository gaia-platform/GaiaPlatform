/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cpptoml.h>

#include "gaia/db/db.hpp"

namespace gaia
{
namespace db
{
namespace config
{

/**
 * Given a toml parsed file returns a session_opts_t. If the configuration
 * does not contain the necessary values, a session_opt_t object will be
 * created with default values.
 */
gaia::db::session_options_t create_session_options(std::shared_ptr<cpptoml::table> root_config);

/**
 * Get the default value for session_opts. This value can be changed by set_default_session_options().
 */
gaia::db::session_options_t get_default_session_options();

/**
 * Change the value returned by get_default_session_options().
 * gaia::system::initialize() calls this method after reading
 * the configuration.
 */
void set_default_session_options(session_options_t session_opts);

} // namespace config
} // namespace db
} // namespace gaia
