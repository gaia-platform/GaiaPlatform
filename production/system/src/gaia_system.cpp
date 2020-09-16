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
}
