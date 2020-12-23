/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia/db/catalog.hpp"
#include "gaia/db/db.hpp"
#include "gaia/exception.hpp"
#include "gaia/exceptions.hpp"
#include "gaia/rules/rules.hpp"
#include "gaia/system.hpp"
#include "cpptoml.h"
#include "logger.hpp"
#include "rules_config.hpp"
#include "scope_guard.hpp"

using namespace std;
using namespace gaia::rules;
using namespace gaia::system;
using namespace gaia::common;
using namespace scope_guard;

namespace gaia
{
namespace system
{

// The system library will read the system level keys.
// Subsystems should manage their own keys and section names.
// See gaia.conf file for expected section and key names.
shared_ptr<cpptoml::table> parse_system_settings(
    const char* gaia_config_file,
    string& logger_config_file)
{
    static const char* c_system_section = "System";
    static const char* c_logger_config_key = "logger_config";
    shared_ptr<cpptoml::table> root_config;

    root_config = cpptoml::parse_file(gaia_config_file);
    if (!root_config)
    {
        return root_config;
    }

    shared_ptr<cpptoml::table> system_config = root_config->get_table(c_system_section);
    if (system_config)
    {
        auto logger_config_setting = system_config->get_as<string>(c_logger_config_key);
        if (logger_config_setting)
        {
            logger_config_file = *logger_config_setting;
        }
    }

    return root_config;
}

bool file_exists(const char* filename)
{
    ifstream the_file(filename);
    return static_cast<bool>(the_file);
}

void error_if_not_exists(const char* filename)
{
    if (!file_exists(filename))
    {
        throw configuration_error(filename);
    }
}

string get_default_conf_file(const char* default_filename)
{
    // Default locations for log files are placed under/opt/gaia/etc/
    static const char* c_default_conf_directory = "/opt/gaia/etc/";
    string str = c_default_conf_directory;
    str.append(default_filename);

    if (!file_exists(str.c_str()))
    {
        // Okay, just return an empty string then.
        str.clear();
    }

    return str;
}

// If the user passed in a filename, then throw an exception if it does not exist.
// If the user did not pass in a filename, then look under /opt/gaia/etc/ for the
// file and see if that exists.  If that file doesn't exist, then continue on
// without throwing an exception.  Components can initialize themselves with
// appropriate defaults.
string get_conf_file(const char* user_filename, const char* default_filename)
{
    string str;

    if (user_filename)
    {
        error_if_not_exists(user_filename);
        str = user_filename;
    }
    else
    {
        str = get_default_conf_file(default_filename);
    }

    return str;
}

} // namespace system
} // namespace gaia

void gaia::system::initialize(const char* gaia_config_file, const char* logger_config_file)
{
    // Default locations for log files are placed under/opt/gaia/etc/
    static const char* c_default_gaia_conf = "gaia.conf";
    static const char* c_default_logger_conf = "gaia_log.conf";
    bool db_initialized = false;
    shared_ptr<cpptoml::table> root_config;

    string gaia_config_str = get_conf_file(gaia_config_file, c_default_gaia_conf);
    string logger_config_str = get_conf_file(logger_config_file, c_default_logger_conf);

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
    gaia::db::end_session();
    gaia_log::shutdown();
}
