/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "gaia_system.hpp"
#include "gaia_db.hpp"
#include "rules.hpp"
#include "gaia_logging.hpp"

void gaia::system::initialize()
{
    // Create the storage engine session first as the event manager depends on it.
    gaia::db::begin_session();

    gaia::rules::initialize_rules_engine();

    // TODO it is likely that the log configuration will not be found.
    //   We need a more resilient mechanism to configure the logging.
    //   For instance a fallback:
    //    1. Search for an environment variable (eg. GAIA_LOG_CONF)
    //    2. Search in some local folders (eg. the current working directory log_conf.toml or some subdirectory config/log_conf.toml)
    //    3. Search in some system paths (eg /etc/gaia/log_conf.toml)
    //    4. You don't find it and fallback to the default configuration.
    gaia_log::init_logging(gaia_log::c_gaia_root_logger);
}
