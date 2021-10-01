/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia/db/db.hpp"
#include "gaia/exception.hpp"
#include "gaia/exceptions.hpp"
#include "gaia/rules/rules.hpp"
#include "gaia/system.hpp"

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/common/config.hpp"
#include "gaia_internal/common/logger_internal.hpp"
#include "gaia_internal/common/scope_guard.hpp"
#include "gaia_internal/db/db_client_config.hpp"
#include "gaia_internal/rules/rules_config.hpp"

#include "cpptoml.h"

using namespace std;
using namespace gaia::rules;
using namespace gaia::system;
using namespace gaia::common;
using namespace scope_guard;

void gaia::system::initialize(const char* gaia_config_file, const char* logger_config_file)
{
    // Default locations for log files are placed under/opt/gaia/etc/
    bool db_initialized = false;
    shared_ptr<cpptoml::table> root_config;

    string gaia_config_str = get_conf_file_path(gaia_config_file, c_default_conf_file_name);
    if (gaia_config_file && gaia_config_str.empty())
    {
        throw configuration_error(gaia_config_file);
    }

    string logger_config_str = get_conf_file_path(logger_config_file, c_default_logger_conf_file_name);
    // An explicitly requested file must exist. Else if the default file doesn't exist, allow the
    // subsystem to adjust (or not).
    if (logger_config_file && logger_config_str.empty())
    {
        throw configuration_error(logger_config_file);
    }

    if (!gaia_config_str.empty())
    {
        root_config = cpptoml::parse_file(gaia_config_str);
    }

    // This root_config can be in one of three states that must be handled by component initialization functions.
    // 1) Null:  No configuration file was passed in and no default could be found.
    // 2) Empty: There is an underlying table but there were no sections or keys in it.
    // 3) Populated:  Underlying table was parsed and there are sections and keys.

    // Init logging first so components have a chance to output any errors they encounter.
    // It is safe to pass this an empty string.
    gaia_log::initialize(logger_config_str);

    // If we fail from this point forward then be sure to cleanup already initialized components.
    auto cleanup_init_state = make_scope_guard([db_initialized] {
        if (db_initialized)
        {
            gaia::db::end_session();
        }
        gaia_log::shutdown();
    });

    if (root_config != nullptr)
    {
        // Sets the default sessions opts with what is specified in the config file.
        gaia::db::config::session_options_t session_options = gaia::db::config::create_session_options(root_config);
        gaia::db::config::set_default_session_options(session_options);
    }

    gaia::db::begin_session();

    // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
    db_initialized = true;

    gaia::catalog::initialize_catalog();
    gaia::rules::initialize_rules_engine(root_config);

    cleanup_init_state.dismiss();
}

void gaia::system::shutdown()
{
    // Shutdown in reverse order of initialization. Shutdown functions should
    // not fail even if the component has not been initialized.
    gaia::rules::shutdown_rules_engine();
    try
    {
        gaia::db::end_session();
    }
    catch (const gaia::db::no_open_session&)
    {
        // Ignore exception; session socket will be closed when thread terminates.
    }
    catch (const gaia::db::transaction_in_progress&)
    {
        // Ignore exception; session socket will be closed when thread terminates.
    }
    gaia_log::shutdown();
}
