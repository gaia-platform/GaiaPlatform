/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_system.hpp"

#include "catalog.hpp"
#include "cpptoml.h"
#include "gaia_db.hpp"
#include "gaia_exception.hpp"
#include "logger.hpp"
#include "rules_config.hpp"
#include "scope_guard.hpp"

using namespace gaia::rules;
using namespace gaia::system;
using namespace gaia::common;
using namespace std;
using namespace scope_guard;

namespace gaia
{
namespace system
{

// The system library will read the system level keys.
// Subsystems should manage their own keys and section names.
// See gaia_confg.toml file for expected section and key names.
shared_ptr<cpptoml::table> parse_system_settings(
    const char* gaia_config_file,
    string& logger_config_file)
{
    static const char* c_system_section = "System";
    static const char* c_logger_config_key = "logger_config";
    shared_ptr<cpptoml::table> root_config;

    if (!gaia_config_file)
    {
        return root_config;
    }

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

} // namespace system
} // namespace gaia

void gaia::system::initialize(const char* gaia_config_file)
{
    string logger_config_file;
    bool db_initialized = false;

    shared_ptr<cpptoml::table> root_config = parse_system_settings(gaia_config_file, logger_config_file);

    // Init logging first so components have a chance to output any errors they encounter.
    gaia_log::initialize(logger_config_file);

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
