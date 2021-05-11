/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/db/db_client_config.hpp"

#include <string>

#include "gaia_internal/common/config.hpp"
#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/gaia_db_internal.hpp"

using std::string;

inline std::unique_ptr<gaia::db::session_options_t> g_default_session_options = nullptr;

gaia::db::session_options_t gaia::db::config::create_session_options(std::shared_ptr<cpptoml::table> root_config)
{
    ASSERT_PRECONDITION(root_config != nullptr, "root_config must be set!");

    string value = root_config
                       ->get_qualified_as<string>(common::c_instance_name_string_key)
                       .value_or(db::c_default_instance_name);

    gaia::db::session_options_t session_options;
    session_options.db_instance_name = value;
    return session_options;
}

gaia::db::session_options_t gaia::db::config::get_default_session_options()
{
    if (g_default_session_options)
    {
        return *g_default_session_options;
    }

    gaia::db::session_options_t session_options;
    session_options.db_instance_name = c_default_instance_name;
    return session_options;
}

void gaia::db::config::set_default_session_options(gaia::db::session_options_t session_options)
{
    g_default_session_options = std::make_unique<gaia::db::session_options_t>(session_options);
}
