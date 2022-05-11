/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cpptoml.h>

#include "gaia/db/db.hpp"

#include "gaia_internal/db/db.hpp"

namespace gaia
{
namespace db
{

namespace config
{

struct session_options_t
{
    std::string db_instance_name;
    bool skip_catalog_integrity_check;
    session_type_t session_type;

    session_options_t()
        : skip_catalog_integrity_check{false}, session_type{session_type_t::regular}
    {
    }
};

/**
 * Given a toml parsed file returns a session_options_t. If the configuration
 * does not contain the necessary values, a session_opt_t object will be
 * created with default values.
 */
session_options_t create_session_options(std::shared_ptr<cpptoml::table> root_config);

/**
 * Get the default value for session_options. This value can be changed by set_default_session_options().
 */
session_options_t get_default_session_options();

/**
 * Change the value returned by get_default_session_options().
 * gaia::system::initialize() calls this method after reading
 * the configuration.
 */
void set_default_session_options(session_options_t session_options);

} // namespace config
} // namespace db
} // namespace gaia
