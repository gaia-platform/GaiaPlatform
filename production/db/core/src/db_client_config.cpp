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

inline std::unique_ptr<gaia::db::session_opts_t> g_default_session_opts = nullptr;

gaia::db::session_opts_t gaia::db::config::create_session_opts(std::shared_ptr<cpptoml::table> root_config)
{
    ASSERT_PRECONDITION(root_config != nullptr, "root_config must be set!");

    string value = root_config->get_qualified_as<string>(common::c_instance_name_string_key)
                       .value_or(db::c_default_instance_name);

    return gaia::db::session_opts_t{
        .instance_name = value};
}

gaia::db::session_opts_t gaia::db::config::get_default_session_opts()
{
    if (g_default_session_opts)
    {
        return *g_default_session_opts;
    }

    return gaia::db::session_opts_t{
        .instance_name = c_default_instance_name};
}

void gaia::db::config::set_default_session_opts(gaia::db::session_opts_t session_opts)
{
    g_default_session_opts = std::make_unique<gaia::db::session_opts_t>(session_opts);
}
