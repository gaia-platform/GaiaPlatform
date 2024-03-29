////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include <cpptoml.h>

#include "gaia/db/db.hpp"
#include "gaia/system.hpp"

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/common/config.hpp"
#include "gaia_internal/common/logger.hpp"
#include "gaia_internal/common/scope_guard.hpp"
#include "gaia_internal/db/db.hpp"
#include "gaia_internal/db/db_client_config.hpp"
#include "gaia_internal/exceptions.hpp"

#if INCLUDE_RULES_ENGINE
#include "gaia/rules/rules.hpp"

#include "gaia_internal/rules/rules_config.hpp"
#endif

using namespace std;
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
        throw configuration_error_internal(gaia_config_file);
    }

    string logger_config_str = get_conf_file_path(logger_config_file, c_default_logger_conf_file_name);
    // An explicitly requested file must exist. Else if the default file doesn't exist, allow the
    // subsystem to adjust (or not).
    if (logger_config_file && logger_config_str.empty())
    {
        throw configuration_error_internal(logger_config_file);
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

    // Start a DDL session for the catalog initialization.
    gaia::db::begin_ddl_session();

    // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
    db_initialized = true;

    gaia::catalog::initialize_catalog();

    // The rules engine worker threads will start their own sessions,
    // so we need to perform this step within a regular database session.
    gaia::db::end_session();
    gaia::db::begin_session();

#if INCLUDE_RULES_ENGINE
    gaia::rules::initialize_rules_engine(root_config);
#endif

    cleanup_init_state.dismiss();
}

void gaia::system::shutdown()
{
    // Shutdown in reverse order of initialization. Shutdown functions should
    // not fail even if the component has not been initialized.
#if INCLUDE_RULES_ENGINE
    gaia::rules::shutdown_rules_engine();
#endif

    try
    {
        // We expect the session opened by gaia::system::initialize() on the
        // main thread to still be open, but don't want to fail if it's not.
        if (gaia::db::is_session_open())
        {
            if (gaia::db::is_transaction_open())
            {
                gaia_log::sys().warn(
                    "A system shutdown was initiated while a transaction was open.");
                gaia::db::rollback_transaction();
            }

            gaia::db::end_session();
        }
    }
    catch (const gaia::common::gaia_exception& e)
    {
        gaia_log::sys().warn(
            "An exception occurred while shutting down the database: '{}'.", e.what());
    }
    gaia_log::shutdown();
}

bool gaia::system::validate_db_schema(const char* database_name, const char* hash)
{
    auto db_iterator = gaia::catalog::gaia_database_t::list().where(gaia::catalog::gaia_database_expr::name == database_name).begin();
    if (db_iterator != gaia::catalog::gaia_database_t::list().end())
    {
        const char* db_hash = db_iterator->hash();
        if (strcmp(db_hash, hash) != 0)
        {
            return false;
        }
    }
    else
    {
        return false;
    }
    return true;
}
